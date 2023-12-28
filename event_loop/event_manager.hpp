#ifndef EVENT_MANAGER_
#define EVENT_MANAGER_

#include <coroutine>
#include <cstddef>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#include "communication/communication_channel.hpp"
#include "communication/communication_types.hpp"
#include "communication/response_packs.hpp"
#include "coroutine/task.hpp"
#include "event_loop/request_data.hpp"
#include "parameter_packs.hpp"

// forward declare the awaitable return types and request data
struct RequestData;
struct ReadAwaitable;
struct WriteAwaitable;
struct ReadvAwaitable;
struct WritevAwaitable;
struct CloseAwaitable;
struct ShutdownAwaitable;
struct AcceptAwaitable;
struct ConnectAwaitable;

struct QueueOperation {
  OperationParameterPackVariant operation;

  bool await_ready() const noexcept { return false; }
  void await_suspend(EvTask::Handle h) {
    h.promise().state.queue_operation_requests.push_back(std::move(operation));
    h.resume();
  }
  void await_resume() {}

  QueueOperation(OperationParameterPackVariant op) : operation(op) {}
};

struct ObtainQueueOperations {
  using RequestVec = std::vector<OperationParameterPackVariant>;
  RequestVec requests{};
  bool await_ready() const noexcept { return false; }
  void await_suspend(EvTask::Handle h) {
    auto &state = h.promise().state;
    requests = std::move(state.queue_operation_requests);
    state.queue_operation_requests = {};
    h.resume();
  }
  RequestVec await_resume() { return std::move(requests); }
};

struct GenericResponse {
  CommunicationChannel *channel{};
  bool await_ready() const noexcept { return false; }
  void await_suspend(EvTask::Handle h) {
    channel = &h.promise().state.com_data;
  }
  CommunicationChannel *await_resume() { return channel; }
};

template <typename F>
concept SubmitAndWaitHandler =
    requires(F fn, RequestType req_type, CommunicationChannel *channel) {
      fn(req_type, channel);
    };

class EventManager {
  enum LivingState { NOT_STARTED, LIVING, DYING, DEAD };
  LivingState manager_life_state_{};

  static std::mutex init_mutex;
  static int shared_ring_fd;
  static int ring_instances;

  io_uring ring{};

  std::vector<EvTask*> coroutines_to_start{};

  void await_message();
  void event_handler(int res, RequestData *req_data);

  std::size_t in_flight_requests{};
  bool should_restrict_usage();
  EvTask kill_coro_task;
  EvTask kill_internal();

public:
  EvTask kill();

  // registering the coroutine puts it in a vector, the vector is looped through
  // in the event loop and emptied, and each coroutine has .start() called on
  // it, so it relies on the coroutines not starting immediately (i.e
  // std::suspend_always initial_suspend())
  void register_coro(EvTask *coro);

  EventManager(size_t queue_depth);

  void start();
  int submit_queued_entries();
  io_uring_sqe *get_uring_sqe();

  ReadAwaitable read(int fd, uint8_t *buffer, size_t length);
  WriteAwaitable write(int fd, const uint8_t *buffer, size_t length);
  CloseAwaitable close(int fd);
  ShutdownAwaitable shutdown(int fd, int how);
  ReadvAwaitable readv(int fd, struct iovec *iovs, size_t num);
  WritevAwaitable writev(int fd, struct iovec *iovs, size_t num);
  AcceptAwaitable accept(int sockfd, sockaddr *addr, socklen_t *addrlen);
  ConnectAwaitable connect(int sockfd, const sockaddr *addr, socklen_t addrlen);

  EvTask queue_read(int fd, uint8_t *buffer, size_t length);
  EvTask queue_write(int fd, const uint8_t *buffer, size_t length);
  EvTask queue_close(int fd);
  EvTask queue_shutdown(int fd, int how);
  EvTask queue_readv(int fd, struct iovec *iovs, size_t num);
  EvTask queue_writev(int fd, struct iovec *iovs, size_t num);
  EvTask queue_accept(int sockfd, sockaddr *addr, socklen_t *addrlen);
  EvTask queue_connect(int sockfd, const sockaddr *addr, socklen_t addrlen);

  EvTask dispatch_requests(ObtainQueueOperations::RequestVec requests_vec);

  template <SubmitAndWaitHandler F> EvTask submit_and_wait(F handler) {
    std::cout << "disaptchin\n";
    ObtainQueueOperations::RequestVec requests_vec =
        co_await ObtainQueueOperations{};
    int num_requests = requests_vec.size();
    co_await dispatch_requests(std::move(requests_vec));

    auto ret = io_uring_submit(&ring);
    if (ret < 0) {
      co_return ret;
    }

    // how many are currently being processed
    auto num_submitted = ret;

    // how many are still queued for submission
    auto num_queued = ring.sq.sqe_tail - ring.sq.sqe_head;

    while (num_submitted != 0 || num_queued != 0) {
      std::cout << "submitting outer " << ret << " out of " << num_requests << "\n";
      while (num_queued != 0 && num_submitted == 0) {
        auto ret = io_uring_submit(&ring);
        std::cout << "submitting inner " << ret << " out of " << num_requests << "\n";
        if (ret < 0) {
          co_return ret;
        }

        num_submitted += ret;
        num_queued -= ret;
      }
    }

    for (int i = 0; i < num_requests; i++) {
      auto channel = co_await GenericResponse{};
      auto response_type = channel->response_store_current_type();
      handler(response_type, channel);
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