#ifndef EVENT_MANAGER_
#define EVENT_MANAGER_

#include <coroutine>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#include "communication/communication_channel.hpp"
#include "communication/request_types.hpp"
#include "communication/response_types.hpp"
#include "coroutine/awaiter.hpp"
#include "coroutine/task.hpp"

#include "io_awaitables.hpp"

class EventManager {
  enum living_state { NOT_STARTED, LIVING, DYING, DEAD };
  living_state manager_life_state_{};

  static std::mutex init_mutex;
  static int shared_ring_fd;
  static int ring_instances;

  io_uring ring{};

  std::vector<EvTask> coroutines_to_start{};

public:
  EvTask kill_ring();

  // registering the coroutine puts it in a vector, the vector is looped through
  // in the event loop and emptied, and each coroutine has .start() called on
  // it, so it relies on the coroutines not starting immediately (i.e
  // std::suspend_always initial_suspend())
  void register_coro(EvTask &&coro);
  void register_coro(EvTask &coro);

  EventManager(size_t queue_depth);

  void start();
  void await_message();
  void event_handler(int res, RequestData *req_data);

  ReadAwaitable read(int fd, uint8_t *buffer, size_t length) {
    return ReadAwaitable{fd, buffer, length, &ring};
  }

  WriteAwaitable write(int fd, uint8_t *buffer, size_t length) {
    return WriteAwaitable{fd, buffer, length, &ring};
  }

  CloseAwaitable close(int fd) {
    return CloseAwaitable{fd, &ring};
  }

  ShutdownAwaitable shutdown(int fd, int how) {
    return ShutdownAwaitable{fd, how, &ring};
  }

  ReadvAwaitable readv(int fd, struct iovec *iovs, size_t num) {
    return ReadvAwaitable{fd, iovs, num, &ring};
  }

  WritevAwaitable writev(int fd, struct iovec *iovs, size_t num) {
    return WritevAwaitable{fd, iovs, num, &ring};
  }

  AcceptAwaitable accept(int sockfd, sockaddr *addr, socklen_t *addrlen) {
    return AcceptAwaitable{sockfd, addr, addrlen, &ring};
  }

  ConnectAwaitable connect(int sockfd, const sockaddr* addr, socklen_t addrlen) {
    return ConnectAwaitable{sockfd, addr, addrlen, &ring};
  }

  // int queue_read(int fd, uint8_t *buffer, size_t length);
  // int queue_readv(int fd, struct iovec *iovs, size_t num);
  // int queue_write(int fd, uint8_t *buffer, size_t length);
  // int queue_writev(int fd, struct iovec *iovs, size_t num);
  // int queue_accept(int fd);
  // int queue_shutdown(int fd, int how);
  // int queue_close(int fd);

  template <typename F> EvTask submit_and_wait(F handler) {
    auto ret = io_uring_submit(&ring);
    if (ret < 0) {
      co_return ret;
    }

    // how many are currently being processed
    auto num_submitted = ret;

    // how many are still queued for submission
    auto num_queued = ring.sq.sqe_tail - ring.sq.sqe_head;

    while (num_submitted != 0 || num_queued != 0) {
      while (num_queued != 0 && num_submitted == 0) {
        auto ret = io_uring_submit(&ring);
        if (ret < 0) {
          co_return ret;
        }

        num_submitted += ret;
        num_queued -= ret;
      }

      // auto channel = co_await GenericResponse{};
      // auto response_type = channel->response_store_current_type();
      // handler(response_type, channel);

      // auto l = []() -> EvTask {
      //   co_await EvAwaiter<RequestType::ACCEPT, ResponseType::ACCEPT>{};
      // };
    }

    co_return 0;
  }

  /*
  - queue functions for merely doing io_uring_get_sqe and preparing the
  appropriate request
  - queueing won't increment any counter, and submitting won't decrement any
  - ring.sq.sqe_tail - ring.sq.sqe_head can be used to get how many entires in
  submission queue
  - submit_and_wait calls io_uring_submit, which submits n items say, we know
  then (ring.sq.sqe_tail - ring.sq.sqe_head) - n items left to submit

  */
};

#endif