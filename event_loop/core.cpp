#include "event_manager.hpp"

int EventManager::shared_ring_fd = -1;
int EventManager::ring_instances{};
std::mutex EventManager::init_mutex{};

void EventManager::start() {
  while (manager_life_state_ != DEAD) {
    // start all queued up coroutines, then empty the vector
    for (auto &coro : coroutines_to_start) {
      auto channel = coro.start();
      jobs.emplace_back(std::move(coro), channel);
    }

    coroutines_to_start.clear();

    // process all jobs then empty the vector
    for (auto &job : jobs) {
      process_job(std::move(job));
    }

    jobs.clear();

    await_message();
  }
}

void EventManager::await_message() {}

void EventManager::process_job(Job &&job) {
  // should call stuff like io_uring_submit
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
  co_return; // should handle all stages of event loop shutdown
};

void EventManager::register_coro(EvTask &&coro) {
  coroutines_to_start.emplace_back(std::move(coro));
}

void EventManager::register_coro(EvTask &coro) { coroutines_to_start.push_back(coro); }