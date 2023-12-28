#ifndef EV_TASK_
#define EV_TASK_

#include "event_loop/parameter_packs.hpp"

#include "communication/communication_channel.hpp"
#include "communication/communication_types.hpp"

#include <coroutine>
#include <iostream>
#include <memory>
#include <vector>

class EvTask {
public:
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;

private:
  bool started_coro{};
  std::unique_ptr<bool> is_done_ptr{};
  std::unique_ptr<int> ret_code_ptr{};
  Handle handle{};

public:
  struct promise_type {
    struct {
      std::exception_ptr exception_ptr{};
      CommunicationChannel com_data{};
      std::coroutine_handle<> awaiter_handle{};
      int *ret_code_ptr{};
      bool *is_done_ptr{};
      
      std::vector<OperationParameterPackVariant> queue_operation_requests{};
    } state;

    EvTask get_return_object() { return EvTask{Handle::from_promise(*this)}; }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept {
      if (state.awaiter_handle) {
        state.awaiter_handle.resume();
      }

      if (state.is_done_ptr) {
        *state.is_done_ptr = true;
      }
      return {};
    }

    void return_value(int ret_code = 0) {
      if (state.ret_code_ptr) {
        *state.ret_code_ptr = ret_code;
      }
    }

    void unhandled_exception() {
      state.exception_ptr = std::current_exception();
    }

    template <RequestType Rt, typename RespType = RespDataTypeMap<Rt>>
    void set_resp_data(RespType &&data) {
      state.com_data.set_resp_data<Rt>(std::forward<RespType>(data));
    }
  };

  EvTask(Handle h) : handle(h) {
    is_done_ptr = std::make_unique<bool>();
    ret_code_ptr = std::make_unique<int>();

    auto &state = h.promise().state;
    state.is_done_ptr = is_done_ptr.get();
    state.ret_code_ptr = ret_code_ptr.get();
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

    if (is_done_ptr && *is_done_ptr) {
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

    com_channel.set_resp_data<RespT>(resp_data);
    handle.resume();
    // point X

    if (state.exception_ptr) {
      std::rethrow_exception(state.exception_ptr);
    }

    return &com_channel;
  }

  bool is_done() { return *is_done_ptr; }

  explicit operator bool() { return *is_done_ptr; }

  // these below are what makes this task awaitable
  bool await_ready() const noexcept { return false; };

  void await_suspend(Handle other_handle) {
    // if the coroutine hasn't started upon co_awaiting, do that first
    if (!started_coro) {
      start();
    }

    // in the off chance we're awaiting on a complete coroutine
    if (is_done_ptr && *is_done_ptr) {
      other_handle.resume();
      return;
    }

    auto &state = this->handle.promise().state;
    auto &awaiter_handle = state.awaiter_handle;
    awaiter_handle = other_handle;
  }
  int await_resume() {
    if (ret_code_ptr) {
      return *ret_code_ptr;
    }
    return -1;
  }

  ~EvTask() {
    if (is_done_ptr && !*is_done_ptr) {
      auto &state = handle.promise().state;
      state.is_done_ptr = nullptr;
      state.ret_code_ptr = nullptr;
    }
  }
};

#endif