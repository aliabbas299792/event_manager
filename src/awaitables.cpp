#include "event_manager.hpp"

#include <cstdarg>
#include <initializer_list>

// TODO: call set_handler_return_and_resume in the event loop where appropriate

void ReadAwaitable::await_suspend(ev_coro_handle h) {
  for (const auto r : reqs) {
    ev->queue_read(r.pfd, r.buffer, r.length, r.additional_info);
  }
  ev->set_handler_return_and_resume(h, ev->submit_all_queued_sqes());
}

constexpr bool ReadAwaitable::await_ready() const noexcept { return false; }

int ReadAwaitable::await_resume() { return promise->return_code; }

void ReadvAwaitable::await_suspend(ev_coro_handle h) {
  for (const auto r : reqs) {
    ev->queue_readv(r.pfd, r.iovs, r.num, r.additional_info);
  }
  ev->set_handler_return_and_resume(h, ev->submit_all_queued_sqes());
}

constexpr bool ReadvAwaitable::await_ready() const noexcept { return false; }

int ReadvAwaitable::await_resume() { return promise->return_code; }

void WriteAwaitable::await_suspend(ev_coro_handle h) {
  for (const auto r : reqs) {
    ev->queue_write(r.pfd, r.buffer, r.length, r.additional_info);
  }
  ev->set_handler_return_and_resume(h, ev->submit_all_queued_sqes());
}

constexpr bool WriteAwaitable::await_ready() const noexcept { return false; }

int WriteAwaitable::await_resume() { return promise->return_code; }

void WritevAwaitable::await_suspend(ev_coro_handle h) {
  for (const auto r : reqs) {
    ev->queue_writev(r.pfd, r.iovs, r.num, r.additional_info);
  }
  ev->set_handler_return_and_resume(h, ev->submit_all_queued_sqes());
}

constexpr bool WritevAwaitable::await_ready() const noexcept { return false; }

int WritevAwaitable::await_resume() { return promise->return_code; }

void AcceptAwaitable::await_suspend(ev_coro_handle h) {
  for (const auto r : reqs) {
    ev->queue_accept(r.listener_pfd, r.additional_info);
  }
  ev->set_handler_return_and_resume(h, ev->submit_all_queued_sqes());
}

constexpr bool AcceptAwaitable::await_ready() const noexcept { return false; }

int AcceptAwaitable::await_resume() { return promise->return_code; }

void ShutdownAwaitable::await_suspend(ev_coro_handle h) {
  for (const auto r : reqs) {
    ev->queue_shutdown(r.pfd, r.how, r.additional_info);
  }
  ev->set_handler_return_and_resume(h, ev->submit_all_queued_sqes());
}

constexpr bool ShutdownAwaitable::await_ready() const noexcept { return false; }

int ShutdownAwaitable::await_resume() { return promise->return_code; }

void CloseAwaitable::await_suspend(ev_coro_handle h) {
  for (const auto r : reqs) {
    ev->queue_close(r.pfd, r.additional_info);
  }
  ev->set_handler_return_and_resume(h, ev->submit_all_queued_sqes());
}

constexpr bool CloseAwaitable::await_ready() const noexcept { return false; }

int CloseAwaitable::await_resume() { return promise->return_code; }

ReadAwaitable event_manager::read(int fd, uint8_t *buffer, size_t length) {
  return ReadAwaitable{this, {{fd, buffer, length}}};
}

ReadvAwaitable event_manager::readv(int fd, struct iovec *iovs, size_t num) {
  return ReadvAwaitable{this, {{fd, iovs, num}}};
}

WriteAwaitable event_manager::write(int fd, uint8_t *buffer, size_t length) {
  return WriteAwaitable{this, {{fd, buffer, length}}};
}

WritevAwaitable event_manager::writev(int fd, struct iovec *iovs, size_t num) {
  return WritevAwaitable{this, {{fd, iovs, num}}};
}

AcceptAwaitable event_manager::accept(int listener_fd) { return AcceptAwaitable{this, {{listener_fd}}}; }

ShutdownAwaitable event_manager::shutdown(int fd, int how) { return ShutdownAwaitable{this, {{fd, how}}}; }

CloseAwaitable event_manager::close(int fd) { return CloseAwaitable{this, {{fd}}}; }

ReadAwaitable event_manager::read(std::initializer_list<ReadRequest> reqs) {
  return ReadAwaitable{this, std::move(reqs)};
}

ReadvAwaitable event_manager::readv(std::initializer_list<ReadvRequest> reqs) {
  return ReadvAwaitable{this, std::move(reqs)};
}

WriteAwaitable event_manager::write(std::initializer_list<WriteRequest> reqs) {
  return WriteAwaitable{this, std::move(reqs)};
}

WritevAwaitable event_manager::writev(std::initializer_list<WritevRequest> reqs) {
  return WritevAwaitable{this, std::move(reqs)};
}

AcceptAwaitable event_manager::accept(std::initializer_list<AcceptRequest> reqs) {
  return AcceptAwaitable{this, std::move(reqs)};
}

ShutdownAwaitable event_manager::shutdown(std::initializer_list<ShutdownRequest> reqs) {
  return ShutdownAwaitable{this, std::move(reqs)};
}

CloseAwaitable event_manager::close(std::initializer_list<CloseRequest> reqs) {
  return CloseAwaitable{this, std::move(reqs)};
}

void event_manager::set_handler_return_and_resume(ev_coro_handle handle, int return_code) {
  handle.promise().return_code = return_code;
  handle.resume();
}