#ifndef EV_TASK_
#define EV_TASK_

#include "communication/communication_channel.hpp"
#include "communication/communication_types.hpp"

#include <coroutine>
#include <iostream>
#include <memory>
#include <optional>

struct TaskStatus {
  bool handler_done{};
  int ret_code{};
};

class EvTask {
public:
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;

private:
  bool _started_coro{};
  std::unique_ptr<TaskStatus> _task_status_ptr{};
  Handle _handle{};

public:
  struct promise_type {
    struct {
      std::exception_ptr exception_ptr{};
      CommunicationChannel com_data{};
      std::coroutine_handle<> awaiter_handle{};
      TaskStatus *task_status_ptr{};

      uint64_t metadata{}; // custom user provided metadata
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

    void unhandled_exception() { state.exception_ptr = std::current_exception(); }

    template <RequestType Rt, typename RespType = RespDataTypeMap<Rt>>
    void publish_resp_data(RespType &&data) {
      state.com_data.publish_resp_data<Rt>(std::forward<RespType>(data));
    }

    bool is_done() {
      if (state.task_status_ptr) {
        return state.task_status_ptr->handler_done;
      }
      return false;
    }
  };

  EvTask(Handle h) : _handle(h) {
    _task_status_ptr = std::make_unique<TaskStatus>();

    auto &state = h.promise().state;
    state.task_status_ptr = _task_status_ptr.get();
  }

  EvTask(EvTask &&other)
      : _started_coro(other._started_coro), _task_status_ptr(std::move(other._task_status_ptr)),
        _handle(std::move(other._handle)) {
    other._task_status_ptr = {};
    other._handle = {};
    other._started_coro = false;

    _handle.promise().state.task_status_ptr = _task_status_ptr.get();
  }

  EvTask &operator=(EvTask &&other) {
    if (this == &other) {
      return *this;
    }

    // handle.done() is undefined if the coroutine is not suspended, and since we don't suspend at
    // final_suspend, so we use our own flag
    if (_handle && !_task_status_ptr->handler_done) {
      _handle.destroy();
    }

    _handle = std::move(other._handle);
    _task_status_ptr = std::move(other._task_status_ptr);
    _started_coro = other._started_coro;

    _handle.promise().state.task_status_ptr = _task_status_ptr.get();

    other._task_status_ptr = nullptr;
    other._handle = {};
    other._started_coro = false;

    return *this;
  }

  int set_coro_metadata(uint64_t metadata) {
    if (!_handle) {
      return -1; // unable to set it as the handle is invalid
    }

    _handle.promise().state.metadata = metadata;
    return 0;
  }

  std::optional<uint64_t> get_coro_metadata() {
    if (!_handle) {
      return std::nullopt;
    }
    return std::make_optional(_handle.promise().state.metadata);
  }

  CommunicationChannel *start() {
    if (_started_coro) {
      std::cerr << "The coroutine was already started, this may be discarding "
                   "some data passed via await\n";
      return nullptr;
    }

    _started_coro = true;

    auto &state = _handle.promise().state;
    auto &com_channel = state.com_data;
    _handle.resume();

    if (_task_status_ptr && _task_status_ptr->handler_done) {
      std::cerr << "Cannot communicate with the coroutine as it has finished "
                   "already\n";
      return nullptr;
    }

    if (state.exception_ptr) {
      std::rethrow_exception(state.exception_ptr);
    }

    return &com_channel;
  }

  void resume() { _handle.resume(); }

  template <RequestType RespT, typename ParamType = RespDataTypeMap<RespT>>
  CommunicationChannel *resume(ParamType resp_data) {
    if (!_started_coro) {
      std::cerr << "We may be passing data to the coroutine which it won't "
                   "read since it hasn't started yet\n";
      _started_coro = true;
    }

    // Called outside the coroutine with the response data you'd like to send to
    // pass to the coroutine (if any)
    // After the coroutine pauses again (at point X) then we may have a request
    // from it
    auto &state = _handle.promise().state;
    auto &com_channel = state.com_data;

    com_channel.publish_resp_data<RespT>(resp_data);
    _handle.resume();
    // point X

    if (state.exception_ptr) {
      std::rethrow_exception(state.exception_ptr);
    }

    return &com_channel;
  }

  bool is_done() { return _task_status_ptr->handler_done; }

  explicit operator bool() { return is_done(); }

  // these below are what makes this task awaitable
  bool await_ready() const noexcept { return false; };

  void await_suspend(Handle other_handle) {
    // if the coroutine hasn't started upon co_awaiting, do that first
    if (!_started_coro) {
      start();
    }

    // in the off chance we're awaiting on a complete coroutine
    if (_task_status_ptr && _task_status_ptr->handler_done) {
      other_handle.resume();
      return;
    }

    auto &state = this->_handle.promise().state;
    auto &awaiter_handle = state.awaiter_handle;
    awaiter_handle = other_handle;
  }
  int await_resume() {
    if (_task_status_ptr) {
      return _task_status_ptr->ret_code;
    }
    return -1;
  }

  ~EvTask() {
    if (_task_status_ptr && !_task_status_ptr->handler_done) {
      _handle.promise().state.task_status_ptr = nullptr;
    }
  }
};

#endif