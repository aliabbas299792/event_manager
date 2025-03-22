#include "task.hpp"

EvTask EvTask::promise_type::get_return_object() {
  return EvTask{Handle::from_promise(*this)};
}

std::suspend_always EvTask::promise_type::initial_suspend() noexcept {
  return {};
}

std::suspend_never EvTask::promise_type::final_suspend() noexcept {
  if (state.awaiter_handle) {
    state.awaiter_handle.resume();
  }

  if (state.task_status_ptr) {
    state.task_status_ptr->handler_done = true;
  }
  return {};
}

void EvTask::promise_type::return_value(uint64_t ret_code) {
  if (state.task_status_ptr) {
    state.task_status_ptr->ret_code = ret_code;
  }
}

void EvTask::promise_type::unhandled_exception() {
  state.exception_ptr = std::current_exception();
}

bool EvTask::promise_type::is_done() {
  if (state.task_status_ptr) {
    return state.task_status_ptr->handler_done;
  }
  return false;
}

EvTask::EvTask(Handle h) : _handle(h) {
  _task_status_ptr = std::make_unique<TaskStatus>();

  auto& state = h.promise().state;
  state.task_status_ptr = _task_status_ptr.get();
}

EvTask::EvTask(EvTask&& other)
    : _started_coro(other._started_coro),
      _task_status_ptr(std::move(other._task_status_ptr)),
      _handle(std::move(other._handle)) {
  other._task_status_ptr = {};
  other._handle = {};
  other._started_coro = false;

  _handle.promise().state.task_status_ptr = _task_status_ptr.get();
}

EvTask& EvTask::operator=(EvTask&& other) {
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

int EvTask::set_coro_metadata(uint64_t metadata) {
  if (!_handle) {
    return -1;  // unable to set it as the handle is invalid
  }

  _handle.promise().state.metadata = metadata;
  return 0;
}

std::optional<uint64_t> EvTask::get_coro_metadata() {
  if (!_handle) {
    return std::nullopt;
  }
  return std::make_optional(_handle.promise().state.metadata);
}

CommunicationChannel* EvTask::start() {
  if (_started_coro) {
    std::cerr << "The coroutine was already started, this may be discarding "
                 "some data passed via await\n";
    return nullptr;
  }

  _started_coro = true;

  auto& state = _handle.promise().state;
  auto& com_channel = state.com_data;
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

void EvTask::resume() {
  _handle.resume();
}

bool EvTask::is_done() {
  return _task_status_ptr->handler_done;
}

// these below are what makes this task awaitable
bool EvTask::await_ready() const noexcept {
  return false;
};

void EvTask::await_suspend(Handle other_handle) {
  // if the coroutine hasn't started upon co_awaiting, do that first
  if (!_started_coro) {
    start();
  }

  // in the off chance we're awaiting on a complete coroutine
  if (_task_status_ptr && _task_status_ptr->handler_done) {
    other_handle.resume();
    return;
  }

  auto& state = this->_handle.promise().state;
  auto& awaiter_handle = state.awaiter_handle;
  awaiter_handle = other_handle;
}

uint64_t EvTask::await_resume() {
  if (_task_status_ptr) {
    return _task_status_ptr->ret_code;
  }
  return -1;
}

EvTask::~EvTask() {
  if (_task_status_ptr && !_task_status_ptr->handler_done) {
    _handle.promise().state.task_status_ptr = nullptr;
  }
}