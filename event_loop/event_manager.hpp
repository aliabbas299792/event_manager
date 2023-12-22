#ifndef EVENT_MANAGER_
#define EVENT_MANAGER_

#include <liburing.h>
#include <mutex>
#include <vector>

#include "communication/communication_channel.hpp"
#include "coroutine/task.hpp"

struct Job {
  EvTask coro;
  CommunicationChannel *channel{};

  Job(EvTask &&coroutine, CommunicationChannel *com_channel)
      : coro(std::move(coroutine)), channel(com_channel) {}
};

class EventManager {
  enum living_state { NOT_STARTED, LIVING, DYING, DEAD };
  living_state manager_life_state_{};

  static std::mutex init_mutex;
  static int shared_ring_fd;
  static int ring_instances;

  io_uring ring{};

  std::vector<EvTask> coroutines_to_start{};
  std::vector<Job> jobs{};

public:
  EvTask kill_ring() {
    co_return; // should handle all stages of event loop shutdown
  };

  // registering the coroutine puts it in a vector, the vector is looped through
  // in the event loop and emptied, and each coroutine has .start() called on it,
  // so it relies on the coroutines not starting immediately
  // (i.e std::suspend_always initial_suspend())
  void register_coro(EvTask &&coro) {
    coroutines_to_start.emplace_back(std::move(coro));
  }

  void register_coro(EvTask &coro) { coroutines_to_start.push_back(coro); }

  EventManager(size_t queue_depth) {
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

  void start() {
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

  void await_message() {}

  void process_job(Job &&job) {
    // should call stuff like io_uring_submit
  }
};

#endif