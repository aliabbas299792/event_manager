#ifndef EV_TASK_
#define EV_TASK_

#include "event_loop/parameter_packs.hpp"

#include "communication/communication_channel.hpp"
#include "communication/communication_types.hpp"

#include <coroutine>
#include <iostream>
#include <memory>
#include <vector>

struct task_status {
  bool handler_done{};
  int ret_code{};
};

class EvTask {
public:
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;

private:
  bool started_coro{};
  std::unique_ptr<task_status> task_status_ptr{};
  Handle handle{};

public:
  struct promise_type {
    struct {
      std::exception_ptr exception_ptr{};
      CommunicationChannel com_data{};
      std::coroutine_handle<> awaiter_handle{};
      task_status *task_status_ptr{};
    } state;

    EvTask get_return_object() { return EvTask{Handle::from_promise(*this)}; }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept {
      if (state.awaiter_handle) {
        state.awaiter_handle.resume();
      }

      if (state.task_status_ptr) {
        state.task_status_ptr->handler_done = true;
      }
      return {};
    }

    void return_value(int ret_code = 0) {
      if (state.task_status_ptr) {
        state.task_status_ptr->ret_code = ret_code;
      }
    }

    void unhandled_exception() {
      state.exception_ptr = std::current_exception();
    }

    template <RequestType Rt, typename RespType = RespDataTypeMap<Rt>>
    void publish_resp_data(RespType &&data) {
      state.com_data.publish_resp_data<Rt>(std::forward<RespType>(data));
    }
  };

  EvTask(Handle h) : handle(h) {
    task_status_ptr = std::make_unique<task_status>();

    auto &state = h.promise().state;
    state.task_status_ptr = task_status_ptr.get();
  }

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

    if (task_status_ptr && task_status_ptr->handler_done) {
      std::cerr << "Cannot communicate with the coroutine as it has finished "
                   "already\n";
      return nullptr;
    }

    if (state.exception_ptr) {
      std::rethrow_exception(state.exception_ptr);
    }

    return &com_channel;
  }

  void resume() { handle.resume(); }

  template <RequestType RespT, typename ParamType = RespDataTypeMap<RespT>>
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

    com_channel.publish_resp_data<RespT>(resp_data);
    handle.resume();
    // point X

    if (state.exception_ptr) {
      std::rethrow_exception(state.exception_ptr);
    }

    return &com_channel;
  }

  bool is_done() { return task_status_ptr->handler_done; }

  explicit operator bool() { return is_done(); }

  // these below are what makes this task awaitable
  bool await_ready() const noexcept { return false; };

  void await_suspend(Handle other_handle) {
    // if the coroutine hasn't started upon co_awaiting, do that first
    if (!started_coro) {
      start();
    }

    // in the off chance we're awaiting on a complete coroutine
    if (task_status_ptr && task_status_ptr->handler_done) {
      other_handle.resume();
      return;
    }

    auto &state = this->handle.promise().state;
    auto &awaiter_handle = state.awaiter_handle;
    awaiter_handle = other_handle;
  }
  int await_resume() {
    if (task_status_ptr) {
      return task_status_ptr->ret_code;
    }
    return -1;
  }

  ~EvTask() {
    if (task_status_ptr && !task_status_ptr->handler_done) {
      handle.promise().state.task_status_ptr = nullptr;
    }
  }
};

#endif