#ifndef EVENT_MANAGER_
#define EVENT_MANAGER_

#include <liburing.h>
#include <mutex>
#include <vector>

#include "communication/communication_channel.hpp"
#include "coroutine/task.hpp"

struct RetrieveHandle {
  EvTask::Handle handle{};
  bool await_ready() const noexcept { return false; }
  void await_suspend(EvTask::Handle h) {
    handle = h;
    h.resume();
  }
  EvTask::Handle await_resume() { return handle; }
};

struct GenericResponse {
  CommunicationChannel *stored_data = nullptr;

  bool await_ready() const noexcept { return false; };
  void await_suspend(typename EvTask::Handle handle) {
    stored_data = &handle.promise().state.com_data;
  }
  CommunicationChannel *await_resume() { return stored_data; }
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
  template <typename Fn>
  EvTask read(int fd, uint8_t *buffer, size_t length, Fn handler) {
    auto handle = co_await RetrieveHandle{};
    std::cout << "gto to qeuue read edn and got an sqe\n";

    auto channel = co_await GenericResponse{};
    co_return 0;
  }
};

#endif