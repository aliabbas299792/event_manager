#ifndef EV_TASK_
#define EV_TASK_

#include "communication/communication_channel.hpp"
#include "communication/communication_types.hpp"

#include <coroutine>
#include <iostream>
#include <vector>

class EvTask {
  bool started_coro{};

public:
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;

  struct promise_type {
    struct {
      std::exception_ptr exception_ptr{};
      CommunicationChannel com_data{};
      std::vector<std::coroutine_handle<>> awaiter_handles{};
      int ret_code{};
    } state;

    EvTask get_return_object() { return EvTask{Handle::from_promise(*this)}; }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept {
      for (auto &h : state.awaiter_handles) {
        h.resume();
      }
      return {};
    }

    void return_value(int ret_code = 0) { state.ret_code = ret_code; }

    void unhandled_exception() {
      state.exception_ptr = std::current_exception();
    }

    template <RequestType Rt, typename RespType = RespTypeMap<Rt>>
    void set_resp_data(RespType &&data) {
      state.com_data.set_resp_data<Rt>(std::forward<RespType>(data));
    }
  };

  Handle handle{};

  EvTask(Handle h) : handle(h) {}

  CommunicationChannel *start() {
    if (started_coro) {
      std::cerr << "The coroutine was already started, this may be discarding "
                   "some data passed via await\n";
      return nullptr;
    }

    started_coro = true;

    auto &state = handle.promise().state;
    auto &com_channel = state.com_data;
    handle.resume();

    if (state.exception_ptr) {
      std::rethrow_exception(state.exception_ptr);
    }

    return &com_channel;
  }

  template <RequestType RespT, typename ParamType = RespTypeMap<RespT>>
  CommunicationChannel *resume(ParamType resp_data) {
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
    auto &com_channel = state.com_data;

    com_channel.set_resp_data<RespT>(resp_data);
    handle.resume();
    // point X

    if (state.exception_ptr) {
      std::rethrow_exception(state.exception_ptr);
    }

    return &com_channel;
  }

  bool is_done() { return handle.done(); }

  explicit operator bool() { return is_done(); }

  // these below are what makes this task awaitable
  bool await_ready() const noexcept {
    return false;
  };

  void await_suspend(Handle other_handle) {
    // if the coroutine hasn't started upon co_awaiting, do that first
    if (!started_coro) {
      start();
    }

    if (is_done()) { // in the off chance we're awaiting on a complete coroutine
      other_handle.resume();
      return;
    }

    auto &state = this->handle.promise().state;
    auto &awaiter_handles = state.awaiter_handles;
    awaiter_handles.push_back(other_handle);
  }
  int await_resume() {
    return handle.promise().state.ret_code;
  }
};

#endif