#include "communication/communication_types.hpp"
#include "event_loop/request_data.hpp"
#include "event_manager.hpp"
#include <cerrno>

int EventManager::shared_ring_fd = -1;
int EventManager::ring_instances{};
std::mutex EventManager::init_mutex{};

void EventManager::start() {
  while (manager_life_state_ != DEAD) {
    // start all queued up coroutines, then empty the vector
    for (auto &coro : coroutines_to_start) {
      coro.start();
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
}

EventManager::EventManager(size_t queue_depth) {
  std::scoped_lock<std::mutex> lock{init_mutex};

  // uses a shared asynchronous backend for all threads
  if (shared_ring_fd == -1 || ring_instances == 0) {
    io_uring_queue_init(queue_depth, &ring, 0);
    shared_ring_fd = ring.ring_fd;
  } else {
    io_uring_params params{};
    params.wq_fd = shared_ring_fd;
    params.flags = IORING_SETUP_ATTACH_WQ;
    io_uring_queue_init_params(queue_depth, &ring, &params);
  }

  register_coro(kill_ring());
}

EvTask EventManager::kill_ring() {
  co_return 0; // should handle all stages of event loop shutdown
};

void EventManager::register_coro(EvTask &&coro) {
  coroutines_to_start.emplace_back(std::move(coro));
}

void EventManager::register_coro(EvTask &coro) {
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

  // if (res < 0) {
  //   std::cerr << "\tio_uring request failure\n";
  // }

  // if (manager_life_state == living_state::DYING_STAGE_2_CANCELLING_REQS) {
  //   dying_stage_2();
  // }
  // pfd_info.submitted_reqs--; // a request for this pfd is no longer active
  // now

  switch (req_data->req_type) {
  case RequestType::READ: {
    ReadResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    } else {
      data = {.bytes_read = res, .buff = specific_data.read_data.buffer};
    }
    promise.set_resp_data<RequestType::READ>(std::move(data));
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
    promise.set_resp_data<RequestType::WRITE>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::CLOSE: {
    CloseResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    }
    promise.set_resp_data<RequestType::CLOSE>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::SHUTDOWN: {
    ShutdownResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    }
    promise.set_resp_data<RequestType::SHUTDOWN>(std::move(data));
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
    promise.set_resp_data<RequestType::READV>(std::move(data));
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
    promise.set_resp_data<RequestType::WRITEV>(std::move(data));
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
    promise.set_resp_data<RequestType::ACCEPT>(std::move(data));
    req_data->handle.resume();
    break;
  }
  case RequestType::CONNECT: {
    ConnectResponsePack data{};
    if (res < 0) {
      data.error_num = errno;
    }
    promise.set_resp_data<RequestType::CONNECT>(std::move(data));
    req_data->handle.resume();
    break;
  }
  }
}