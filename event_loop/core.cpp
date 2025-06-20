#include "communication/communication_types.hpp"
#include "communication/response_packs.hpp"
#include "event_loop/parameter_packs.hpp"
#include "event_loop/request_data.hpp"
#include "event_manager.hpp"
#include <coroutine>
#include <cstdio>
#include <liburing.h>

int EventManager::shared_ring_fd = -1;
size_t EventManager::ring_instances{};
std::mutex EventManager::init_mutex{};

// for stuff like submissions, repeat up to MAX_ITER times,
// and then fail with a message
constexpr const size_t MAX_ITER = 1000;

void EventManager::start() {
  if (_manager_life_state >= LivingState::LIVING) {
    std::cerr << "The event manager is past starting\n";
    if (_manager_life_state > LivingState::LIVING) {
      std::cerr << "In fact it's no longer alive\n";
    }
    return;
  }

  _manager_life_state = LivingState::LIVING;
  while (_manager_life_state < DEAD) {
    await_message();
  }
}

void EventManager::await_message() {
  if (_ready_requests_store.size() != 0 && _polling_handle != nullptr) {
    auto [res, req_data] = _ready_requests_store.back();
    _ready_requests_store.pop_back();
    req_data->handle = _polling_handle;
    event_handler(res, req_data);
  }

  io_uring_cqe* cqe;
  int ret = io_uring_wait_cqe(&_ring, &cqe);

  if (ret < 0) {
    perror("io_uring_wait_cqe");
  }

  auto req_data = reinterpret_cast<RequestData*>(io_uring_cqe_get_data(cqe));
  event_handler(cqe->res, req_data);

  io_uring_cqe_seen(&_ring, cqe);
  _in_flight_requests--;  // seen one request

  // if the kill process has been started, then we must want an update
  if (_manager_life_state == LivingState::DYING) {
    _kill_coro_task.resume();
  }
}

EventManager::EventManager(size_t queue_depth) : _ready_requests_store({}), _kill_coro_task(kill_internal()) {
  std::scoped_lock<std::mutex> lock{init_mutex};

  // uses a shared asynchronous backend for all threads
  if (shared_ring_fd == -1 || ring_instances == 0) {
    int ret = io_uring_queue_init(queue_depth, &_ring, 0);
    if (ret < 0) {
      perror("The IO ring was unable to setup properly (initial _ring for "
             "sharing)");
      perror("The IO ring was unable to be setup properly");
      _manager_life_state = LivingState::DEAD;
      return;
    }

    shared_ring_fd = _ring.ring_fd;
  } else {
    io_uring_params params{};
    params.wq_fd = shared_ring_fd;
    params.flags = IORING_SETUP_ATTACH_WQ;
    int ret = io_uring_queue_init_params(queue_depth, &_ring, &params);
    if (ret < 0) {
      perror("The IO _ring was unable to be setup properly");
      _manager_life_state = LivingState::DEAD;
      return;
    }
  }

  ring_instances++;
}

/*
Let PENDING=(_ring.sq.sqe_tail - _ring.sq.sqe_head)
The number of in flight io_uring operations is tracked using in_flight_requests

1. ev.kill() is called, which does this:
2. We store the handle to the coroutine, and the
   _manager_life_state is updated to be DYING
  - This blocks all new queueing and submissions for the event loop
3. A cancel operation for cancelling any and all operations is submitted
4. As a result in_flight_requests will reach 0 eventually
5. Then the io_uring queue is shutdown

Since this is a coroutine, it can be awaited in another coroutine,
and so we can schedule something to run immediately after
*/
EvTask EventManager::kill_internal() {
  if (_manager_life_state >= LivingState::DYING) {
    std::cerr << "The event manager is already in the process of dying\n";
    co_return -1;
  }

  // disallow new entries
  _manager_life_state = LivingState::DYING;

  // flush the submission queue
  size_t iter = 0;
  while (_ring.sq.sqe_tail - _ring.sq.sqe_head != 0 && iter++ < MAX_ITER) {
    submit_queued_entries();
  }

  if (_ring.sq.sqe_tail - _ring.sq.sqe_head != 0) {
    std::cerr << "[cancel_all_requests]: The submission queue was unable to be "
                 "flushed\n";
    co_return -1;
  }

  // attempt to get an SQE
  io_uring_sqe* sqe{};
  iter = 0;
  while (sqe == nullptr && iter++ < MAX_ITER) {
    sqe = io_uring_get_sqe(&_ring);
  }

  if (sqe == nullptr) {
    std::cerr << "[cancel_all_requests]: Unable to get an SQE\n";
    co_return -1;
  }

  io_uring_prep_cancel(sqe, nullptr, IORING_ASYNC_CANCEL_ANY | IORING_ASYNC_CANCEL_ALL);

  iter = 0;
  while (_ring.sq.sqe_tail - _ring.sq.sqe_head != 0 && iter++ < MAX_ITER) {
    submit_queued_entries();
  }

  if (_ring.sq.sqe_tail - _ring.sq.sqe_head != 0) {
    std::cerr << "[cancel_all_requests]: The submission queue was unable to be "
                 "flushed\n";
    co_return -1;
  }

  // while there are still in flight requests we do not proceed
  while (_in_flight_requests != 0) {
    co_await std::suspend_always{};
  }

  _manager_life_state = LivingState::DEAD;

  io_uring_queue_exit(&_ring);
  ring_instances--;

  if (ring_instances == 0) {
    // reset it since it no longer refers to a valid _ring
    shared_ring_fd = -1;
  }

  co_return 0;
};

EvTask EventManager::kill() {
  co_return co_await _kill_coro_task;
}

void EventManager::event_handler(int res, RequestData* req_data) {
  // don't have anything to process for requests with no data
  if (req_data == nullptr) {
    return;
  }

  if (req_data->handle == nullptr) {
    _ready_requests_store.emplace_back(res, req_data);
    return;
  }

  auto& promise = req_data->handle.promise();
  auto& specific_data = req_data->specific_data;

  // error num is less than 0 when there's an error, otherwise > 0 is i.e. bytes read or whatever
  int error_num = 0;
  if (res < 0) {
    error_num = -res;  // -res since errno isn't used for io_uring
  }

  if (res < 0) {
    std::cerr << "\tio_uring request failure\n";
  }

  switch (req_data->req_type) {
  case RequestType::READ: {
    ReadResponsePack data{};
    if (res >= 0) {
      data = {.bytes_read = static_cast<size_t>(res), .buff = specific_data.read_data.buffer};
    }
    data.error_num = error_num;
    data.req_fd = specific_data.read_data.fd;
    promise.publish_resp_data<RequestType::READ>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::WRITE: {
    WriteResponsePack data{};
    if (res >= 0) {
      data = {.bytes_wrote = static_cast<size_t>(res)};
    }
    data.error_num = error_num;
    data.req_fd = specific_data.write_data.fd;
    promise.publish_resp_data<RequestType::WRITE>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::CLOSE: {
    CloseResponsePack data{};
    data.error_num = error_num;
    data.req_fd = specific_data.close_data.fd;
    promise.publish_resp_data<RequestType::CLOSE>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::SHUTDOWN: {
    ShutdownResponsePack data{};
    data.error_num = error_num;
    data.req_fd = specific_data.shutdown_data.fd;
    promise.publish_resp_data<RequestType::SHUTDOWN>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::READV: {
    ReadvResponsePack data{};
    if (res >= 0) {
      uint8_t* buffer = nullptr;
      if (specific_data.readv_data.iovs && specific_data.readv_data.num > 0) {
        buffer = static_cast<uint8_t*>(specific_data.readv_data.iovs[0].iov_base);
      }
      data = {.bytes_read = static_cast<size_t>(res), .buff = buffer};
    }
    data.error_num = error_num;
    data.req_fd = specific_data.readv_data.fd;
    promise.publish_resp_data<RequestType::READV>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::WRITEV: {
    WritevResponsePack data{};
    if (res >= 0) {
      data = {.bytes_wrote = static_cast<size_t>(res)};
    }
    data.error_num = error_num;
    data.req_fd = specific_data.writev_data.fd;
    promise.publish_resp_data<RequestType::WRITEV>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::ACCEPT: {
    AcceptResponsePack data{};
    if (res >= 0) {
      data = {.fd = res};
    }
    data.error_num = error_num;
    data.req_fd = specific_data.accept_data.sockfd;
    promise.publish_resp_data<RequestType::ACCEPT>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::CONNECT: {
    ConnectResponsePack data{};
    data.error_num = error_num;
    data.req_fd = specific_data.connect_data.sockfd;
    promise.publish_resp_data<RequestType::CONNECT>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::OPENAT: {
    OpenatResponsePack data{};
    data.error_num = error_num;
    data.req_fd = res;
    promise.publish_resp_data<RequestType::OPENAT>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::STATX: {
    StatxResponsePack data{};
    data.error_num = error_num;
    data.pathname = specific_data.statx_data.pathname;
    data.req_fd = -1;  // fd is irrelevant for this operation
    promise.publish_resp_data<RequestType::STATX>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::UNLINKAT: {
    UnlinkatResponsePack data{};
    data.error_num = error_num;
    data.pathname = specific_data.unlinkat_data.pathname;
    data.req_fd = -1;  // fd is irrelevant for this operation
    promise.publish_resp_data<RequestType::UNLINKAT>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::RENAMEAT: {
    RenameatResponsePack data{};
    data.error_num = error_num;
    data.oldpathname = specific_data.renameat_data.oldpathname;
    data.newpathname = specific_data.renameat_data.newpathname;
    data.req_fd = -1;  // fd is irrelevant for this operation
    promise.publish_resp_data<RequestType::RENAMEAT>(std::move(data));
    req_data->handle.resume();
    break;
  }
  }

  // since the tasks final_suspend returns std::suspend_never
  // then req_data->handle.done() is undefined (as handle.done() is only
  // valid for suspended coroutines), so we use our own flags
  if (!req_data->handle || promise.is_done()) {
    // free the index if the coroutine has finished
    _managed_coroutines_store.erase(req_data->coro_idx);
  }

  if (req_data->allocated_dynamic) {
    delete req_data;
  }
}

bool EventManager::should_restrict_usage() {
  if (_manager_life_state > LivingState::LIVING) {
    std::cerr << "The manager is no longer living\n";
    return true;
  }
  return false;
}

int EventManager::submit_queued_entries() {
  // don't need to restrict usage of this since it won't change
  // how many entries are in the SQ, CQ or currently in use
  auto ret = io_uring_submit(&_ring);
  if (ret > 0) {
    _in_flight_requests += ret;
  }
  return ret;
}

void EventManager::register_coro(EvTask&& coro) {
  coro.start();  // start it in case it hasn't been started yet

  uint64_t selected_idx = _managed_coroutines_store.insert(std::move(coro));

  // we are storing the index in the vector as metadata
  _managed_coroutines_store[selected_idx].set_coro_metadata(selected_idx);
}

io_uring_sqe* EventManager::get_uring_sqe() {
  if (should_restrict_usage())
    return nullptr;
  return io_uring_get_sqe(&_ring);
}
