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
  EvTask kill_ring();

  // registering the coroutine puts it in a vector, the vector is looped through
  // in the event loop and emptied, and each coroutine has .start() called on it,
  // so it relies on the coroutines not starting immediately
  // (i.e std::suspend_always initial_suspend())
  void register_coro(EvTask &&coro);
  void register_coro(EvTask &coro);

  EventManager(size_t queue_depth);

  void start();
  void await_message();
  void process_job(Job &&job);
};

#endif