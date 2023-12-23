#include "communication/response_types.hpp"
#include "event_manager.hpp"

int EventManager::shared_ring_fd = -1;
int EventManager::ring_instances{};
std::mutex EventManager::init_mutex{};

/*
Current flow:
1. Enter coroutine
2. Call co_await queue_read() or something
  - This will make a heap object and SQE and set the data
3. Call submit_and_wait(somefn)
  - This submits io_uring requests
  - And in a loop calls co_await GenericResponse{}
  - Which will suspend, and be resumed from incoming requests from event loop
  - And return a communication channel which is passed onto somefn()
    - somefn() may or may not be a coroutine - for now assume it is not

So:
1. Coroutine needs to be registered with event loop
2. Coroutine is started
3. I/O requests may be made inside coroutine, io_uring_wait_cqe gets them when
they are done
4. The handle field of the data request struct has the handle to be resumed
  - Set the correct response data, and call handle.resume<T>()

Right now await_single_message isn't done, and io_uring_wait_cqe isn't added,
add these and add the handling logic for read ReqType Then try out a simple
example of co_await a read request
*/

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
  if (cqe->res < 0) {
    std::cerr << "\tio_uring request failure\n";
  }

  event_handler(cqe->res, req_data);

  io_uring_cqe_seen(&ring, cqe);
}

EventManager::EventManager(size_t queue_depth) {
  std::scoped_lock<std::mutex> lock{init_mutex};

  if (shared_ring_fd == -1 ||
      ring_instances ==
          0) { // uses a shared asynchronous backend for all threads
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
  if (req_data ==
      nullptr) { // don't have anything to process for requests with no data
    return;
  }

  // if (manager_life_state == living_state::DYING_STAGE_2_CANCELLING_REQS) {
  //   dying_stage_2();
  // }
  // pfd_info.submitted_reqs--; // a request for this pfd is no longer active
  // now

  switch (req_data->req_type) {
  case ReqType::WRITE: {
    // callbacks->write_callback(processed_data(req_data->buffer, res,
    // req_data->length), pfd, req_data->additional_info);
    break;
  }
  case ReqType::READ: {
    std::cout << "got to here\n";
    req_data->handle.promise().state.com_data.set_resp_data<ResponseType::READ>(res);
    if(req_data->handle) {
      std::cout << "referst to a real coro\n";
      std::cout << req_data->handle.done() << " (is coro done)\n";
      req_data->handle.resume();
    }else {
      std::cout << "refers to a not coro\n";
    }
    // if (req_data->length == 0) {
    //   // so either from here it submits a shutdown request, or a close
    //   request - after this switch we call the close_pfd function
    //   pfd_info.last_read_zero = true;

    //   // only close the pfd if the user explicitly tells you to
    //   // also only call close if the state has changed, not if there was a
    //   duplicate call if(pfd_info.shutdown_done) {
    //     close_pfd(pfd, req_data->additional_info);
    //   }
    // }

    // if (req_data->ev != ReqType::READ_INTERNAL) {
    //   callbacks->read_callback(processed_data(req_data->buffer, res,
    //   req_data->length), pfd,
    //                            req_data->additional_info);
    // }
    break;
  default:
    std::cout << "got to default\n";
  }
  }
}