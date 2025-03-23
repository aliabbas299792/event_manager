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
  uint64_t ret_code{};
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
      TaskStatus* task_status_ptr{};

      uint64_t metadata{};  // custom user provided metadata
    } state;

    EvTask get_return_object();
    std::suspend_always initial_suspend() noexcept;
    std::suspend_never final_suspend() noexcept;
    void return_value(uint64_t ret_code = 0);
    void unhandled_exception();
    bool is_done();

    template <RequestType Rt, typename RespType = RespDataTypeMap<Rt>>
    void publish_resp_data(RespType&& data) {
      state.com_data.publish_resp_data<Rt>(std::forward<RespType>(data));
    }
  };

  EvTask(Handle h);
  EvTask(EvTask&& other);
  EvTask& operator=(EvTask&& other);
  int set_coro_metadata(uint64_t metadata);
  std::optional<uint64_t> get_coro_metadata();
  CommunicationChannel* start();
  void resume();

  template <RequestType RespT, typename ParamType = RespDataTypeMap<RespT>>
  CommunicationChannel* resume(ParamType resp_data) {
    if (!_started_coro) {
      std::cerr << "We may be passing data to the coroutine which it won't "
                   "read since it hasn't started yet\n";
      _started_coro = true;
    }

    // Called outside the coroutine with the response data you'd like to send to
    // pass to the coroutine (if any)
    // After the coroutine pauses again (at point X) then we may have a request
    // from it
    auto& state = _handle.promise().state;
    auto& com_channel = state.com_data;

    com_channel.publish_resp_data<RespT>(resp_data);
    _handle.resume();
    // point X

    if (state.exception_ptr) {
      std::rethrow_exception(state.exception_ptr);
    }

    return &com_channel;
  }

  bool is_done();
  explicit operator bool() {
    return is_done();
  }

  // below are what makes this task awaitable
  bool await_ready() const noexcept;
  void await_suspend(Handle other_handle);
  uint64_t await_resume();
  ~EvTask();
};

#endif