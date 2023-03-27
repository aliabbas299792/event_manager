#ifndef EVENT_MANAGER_AWAITABLE_TYPES
#define EVENT_MANAGER_AWAITABLE_TYPES

#include <cstdint>
#include <cstddef>
#include <coroutine>
#include <vector>

class event_manager;

struct ReadAwaitable;
struct ReadvAwaitable;
struct WriteAwaitable;
struct WritevAwaitable;
struct AcceptAwaitable;
struct ShutdownAwaitable;
struct CloseAwaitable;

struct ReadRequest {
  int pfd;
  uint8_t *buffer;
  size_t length;
  uint64_t additional_info;
};

struct ReadvRequest {
  int pfd;
  struct iovec *iovs;
  size_t num;
  uint64_t additional_info;
};

struct WriteRequest {
  int pfd;
  uint8_t *buffer;
  size_t length;
  uint64_t additional_info;
};

struct WritevRequest {
  int pfd;
  struct iovec *iovs;
  size_t num;
  uint64_t additional_info;
};

struct AcceptRequest {
  int listener_pfd;
  uint64_t additional_info;
};

struct ShutdownRequest {
  int pfd;
  int how;
  uint64_t additional_info;
};

struct CloseRequest {
  int pfd;
  uint64_t additional_info;
};

struct EventManagerTask {
  struct ev_task_promise_type {
    int return_code{};

    using Handle = std::coroutine_handle<ev_task_promise_type>;

    EventManagerTask get_return_object() { return EventManagerTask{Handle::from_promise(*this)}; }

    // don't return to caller initially
    std::suspend_never initial_suspend() { return {}; }

    // also don't return to caller before the end
    std::suspend_never final_suspend() noexcept { return {}; }

    void return_void() {}
    void unhandled_exception() {}
  };

  explicit EventManagerTask(ev_task_promise_type::Handle coro) : coroutine_handle(coro) {}
  void destroy() { coroutine_handle.destroy(); }
  void resume() { coroutine_handle.resume(); }

private:
  ev_task_promise_type::Handle coroutine_handle;
};

using ev_task_promise_type = EventManagerTask::ev_task_promise_type;
using ev_coro_handle = std::coroutine_handle<ev_task_promise_type>;

struct ReadAwaitable {
  event_manager *ev;
  std::initializer_list<ReadRequest> reqs{};

  ev_coro_handle h{};
  ev_task_promise_type *promise{};

  constexpr bool await_ready() const noexcept;
  void await_suspend(ev_coro_handle h);
  int await_resume();
};

struct ReadvAwaitable {

  event_manager *ev;
  std::initializer_list<ReadvRequest> reqs{};

  ev_coro_handle h{};
  ev_task_promise_type *promise{};

  constexpr bool await_ready() const noexcept;
  void await_suspend(ev_coro_handle h);
  int await_resume();
};

struct WriteAwaitable {

  event_manager *ev;
  std::initializer_list<WriteRequest> reqs{};

  ev_coro_handle h{};
  ev_task_promise_type *promise{};

  constexpr bool await_ready() const noexcept;
  void await_suspend(ev_coro_handle h);
  int await_resume();
};

struct WritevAwaitable {

  event_manager *ev;
  std::initializer_list<WritevRequest> reqs{};

  ev_coro_handle h{};
  ev_task_promise_type *promise{};

  constexpr bool await_ready() const noexcept;
  void await_suspend(ev_coro_handle h);
  int await_resume();
};

struct AcceptAwaitable {

  event_manager *ev;
  std::initializer_list<AcceptRequest> reqs{};

  ev_coro_handle h{};
  ev_task_promise_type *promise{};

  constexpr bool await_ready() const noexcept;
  void await_suspend(ev_coro_handle h);
  int await_resume();
};

struct ShutdownAwaitable {

  event_manager *ev;
  std::initializer_list<ShutdownRequest> reqs{};

  ev_coro_handle h{};
  ev_task_promise_type *promise{};

  constexpr bool await_ready() const noexcept;
  void await_suspend(ev_coro_handle h);
  int await_resume();
};

struct CloseAwaitable {

  event_manager *ev;
  std::initializer_list<CloseRequest> reqs{};

  ev_coro_handle h{};
  ev_task_promise_type *promise{};

  constexpr bool await_ready() const noexcept;
  void await_suspend(ev_coro_handle h);
  int await_resume();
};

#endif