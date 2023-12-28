#include "communication/communication_types.hpp"
#include "event_loop/request_data.hpp"
#include "event_manager.hpp"
#include <cerrno>
#include <coroutine>
#include <cstdio>
#include <liburing.h>

int EventManager::shared_ring_fd = -1;
int EventManager::ring_instances{};
std::mutex EventManager::init_mutex{};

// for stuff like submissions, repeat up to MAX_ITER times,
// and then fail with a message
constexpr const int MAX_ITER = 1000;

void EventManager::start() {
  if (manager_life_state_ >= LivingState::LIVING) {
    std::cerr << "The event manager is past starting\n";
    if (manager_life_state_ > LivingState::LIVING) {
      std::cerr << "In fact it's no longer alive\n";
    }
    return;
  }

  manager_life_state_ = LivingState::LIVING;
  while (manager_life_state_ < DEAD) {
    // start all queued up coroutines, then empty the vector
    for (auto &coro : coroutines_to_start) {
      coro->start();
    }

    coroutines_to_start.clear();

    await_message();
  }
}

void EventManager::await_message() {
  io_uring_cqe *cqe;
  int ret = io_uring_wait_cqe(&ring, &cqe);

  if (ret < 0) {
    perror("io_uring_wait_cqe");
  }

  auto req_data = reinterpret_cast<RequestData *>(io_uring_cqe_get_data(cqe));
  event_handler(cqe->res, req_data);

  io_uring_cqe_seen(&ring, cqe);
  in_flight_requests--; // seen one request

  // if the kill process has been started, then we must want an update
  if (manager_life_state_ == LivingState::DYING) {
    kill_coro_task.resume();
  }
}

EventManager::EventManager(size_t queue_depth)
    : kill_coro_task(kill_internal()) {
  std::scoped_lock<std::mutex> lock{init_mutex};

  // uses a shared asynchronous backend for all threads
  if (shared_ring_fd == -1 || ring_instances == 0) {
    int ret = io_uring_queue_init(queue_depth, &ring, 0);
    if (ret < 0) {
      perror("The IO ring was unable to setup properly (initial ring for "
             "sharing)");
      perror("The IO ring was unable to be setup properly");
      manager_life_state_ = LivingState::DEAD;
      return;
    }

    shared_ring_fd = ring.ring_fd;
  } else {
    io_uring_params params{};
    params.wq_fd = shared_ring_fd;
    params.flags = IORING_SETUP_ATTACH_WQ;
    int ret = io_uring_queue_init_params(queue_depth, &ring, &params);
    if (ret < 0) {
      perror("The IO ring was unable to be setup properly");
      manager_life_state_ = LivingState::DEAD;
      return;
    }
  }

  ring_instances++;
}

/*
Let PENDING=(ring.sq.sqe_tail - ring.sq.sqe_head)
The number of in flight io_uring operations is tracked using in_flight_requests

1. ev.kill() is called, which does this:
2. We store the handle to the coroutine, and the
   manager_life_state_ is updated to be DYING
  - This blocks all new queueing and submissions for the event loop
3. A cancel operation for cancelling any and all operations is submitted
4. As a result in_flight_requests will reach 0 eventually
5. Then the io_uring queue is shutdown

Since this is a coroutine, it can be awaited in another coroutine,
and so we can schedule something to run immediately after
*/
EvTask EventManager::kill_internal() {
  if (manager_life_state_ >= LivingState::DYING) {
    std::cerr << "The event manager is already in the process of dying\n";
    co_return -1;
  }

  // disallow new entries
  manager_life_state_ = LivingState::DYING;

  // flush the submission queue
  int iter = 0;
  while (ring.sq.sqe_tail - ring.sq.sqe_head != 0 && iter++ < MAX_ITER) {
    submit_queued_entries();
  }

  if (ring.sq.sqe_tail - ring.sq.sqe_head != 0) {
    std::cerr << "[cancel_all_requests]: The submission queue was unable to be "
                 "flushed\n";
    co_return -1;
  }

  // attempt to get an SQE
  io_uring_sqe *sqe{};
  iter = 0;
  while (sqe == nullptr && iter++ < MAX_ITER) {
    sqe = io_uring_get_sqe(&ring);
  }

  if (sqe == nullptr) {
    std::cerr << "[cancel_all_requests]: Unable to get an SQE\n";
    co_return -1;
  }

  io_uring_prep_cancel(sqe, nullptr,
                       IORING_ASYNC_CANCEL_ANY | IORING_ASYNC_CANCEL_ALL);

  iter = 0;
  while (ring.sq.sqe_tail - ring.sq.sqe_head != 0 && iter++ < MAX_ITER) {
    submit_queued_entries();
  }

  if (ring.sq.sqe_tail - ring.sq.sqe_head != 0) {
    std::cerr << "[cancel_all_requests]: The submission queue was unable to be "
                 "flushed\n";
    co_return -1;
  }

  // while there are still in flight requests we do not proceed
  while (in_flight_requests != 0) {
    co_await std::suspend_always{};
  }

  manager_life_state_ = LivingState::DEAD;

  io_uring_queue_exit(&ring);
  ring_instances--;

  if (ring_instances == 0) {
    // reset it since it no longer refers to a valid ring
    shared_ring_fd = -1;
  }

  co_return 0;
};

EvTask EventManager::kill() { co_return co_await kill_coro_task; }

void EventManager::register_coro(EvTask *coro) {
  if (should_restrict_usage())
    return;
  coroutines_to_start.push_back(coro);
}

void EventManager::event_handler(int res, RequestData *req_data) {
  // don't have anything to process for requests with no data
  // or if the handle is a nullptr, then there's nothing to resume
  if (req_data == nullptr || req_data->handle == nullptr) {
    return;
  }

  auto &promise = req_data->handle.promise();
  auto &specific_data = req_data->specific_data;

  if (res < 0) {
    std::cerr << "\tio_uring request failure\n";
  }

  switch (req_data->req_type) {
  case RequestType::READ: {
    ReadResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    } else {
      data = {.bytes_read = res, .buff = specific_data.read_data.buffer};
    }
    data.fd = specific_data.read_data.fd;
    promise.publish_resp_data<RequestType::READ>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::WRITE: {
    WriteResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    } else {
      data = {.bytes_wrote = res};
    }
    data.fd = specific_data.write_data.fd;
    promise.publish_resp_data<RequestType::WRITE>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::CLOSE: {
    CloseResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    }
    data.fd = specific_data.close_data.fd;
    promise.publish_resp_data<RequestType::CLOSE>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::SHUTDOWN: {
    ShutdownResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    }
    data.fd = specific_data.shutdown_data.fd;
    promise.publish_resp_data<RequestType::SHUTDOWN>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::READV: {
    ReadvResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    } else {
      data = {.bytes_read = res, .buff = specific_data.read_data.buffer};
    }
    data.fd = specific_data.readv_data.fd;
    promise.publish_resp_data<RequestType::READV>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::WRITEV: {
    WritevResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    } else {
      data = {.bytes_wrote = res};
    }
    data.fd = specific_data.writev_data.fd;
    promise.publish_resp_data<RequestType::WRITEV>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::ACCEPT: {
    AcceptResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    } else {
      data = {.fd = res};
    }
    data.fd = specific_data.accept_data.sockfd;
    promise.publish_resp_data<RequestType::ACCEPT>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::CONNECT: {
    ConnectResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    }
    data.fd = specific_data.accept_data.sockfd;
    promise.publish_resp_data<RequestType::CONNECT>(std::move(data));
    req_data->handle.resume();
    break;
  }
  }
}

bool EventManager::should_restrict_usage() {
  if (manager_life_state_ > LivingState::LIVING) {
    std::cerr << "The manager is no longer living\n";
    return true;
  }
  return false;
}

int EventManager::submit_queued_entries() {
  // don't need to restrict usage of this since it won't change
  // how many entries are in the SQ, CQ or currently in use
  auto ret = io_uring_submit(&ring);
  if (ret > 0) {
    in_flight_requests += ret;
  }
  return ret;
}

io_uring_sqe *EventManager::get_uring_sqe() {
  if (should_restrict_usage())
    return nullptr;
  return io_uring_get_sqe(&ring);
}