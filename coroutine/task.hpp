#ifndef EV_TASK_
#define EV_TASK_

#include "communication/communication_channel.hpp"

#include <coroutine>
#include <iostream>

struct EvTask {
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;
  bool started_coro{};

  struct promise_type {
    struct {
      std::exception_ptr exception_ptr{};
      CommunicationChannel com_data{};
    } state;

    EvTask get_return_object() { return EvTask{Handle::from_promise(*this)}; }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }

    void return_void() {}

    void unhandled_exception() {
      state.exception_ptr = std::current_exception();
    }
  };

  Handle handle{};

  EvTask(Handle h) : handle(h) {}

  CommunicationChannel &start() {
    if (started_coro) {
      std::cerr << "The coroutine was already started, this may be discarding "
                   "some data passed via await\n";
    }

    started_coro = true;

    auto &state = handle.promise().state;
    auto &com_data = state.com_data;
    handle.resume();

    if (state.exception_ptr) {
      std::rethrow_exception(state.exception_ptr);
    }

    return com_data;
  }

  template <ResponseType RespT, typename ParamType = RespTypeMap<RespT>>
  CommunicationChannel &resume(ParamType resp_data) {
    if (!started_coro) {
      std::cerr << "We may be passing data to the coroutine which it won't "
                   "read since it hasn't started yet\n";
      started_coro = true;
    }

    // Called outside the coroutine with the response data you'd like to send to
    // pass to the coroutine (if any)
    // After the coroutine pauses again (at point X) then we may have a request
    // from it
    auto &state = handle.promise().state;
    auto &com_data = state.com_data;

    com_data.set_resp_data<RespT>(resp_data);
    handle.resume();
    // point X

    if (state.exception_ptr) {
      std::rethrow_exception(state.exception_ptr);
    }

    return com_data;
  }

  bool is_done() { return handle.done(); }

  explicit operator bool() { return is_done(); }
};

#endif