#ifndef EVENT_MANAGER_
#define EVENT_MANAGER_

#include <coroutine>
#include <cstddef>
#include <functional>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>
#include <variant>
#include <vector>

#include "communication/communication_channel.hpp"
#include "communication/communication_types.hpp"
#include "communication/response_packs.hpp"
#include "coroutine/task.hpp"
#include "event_loop/request_data.hpp"
#include "parameter_packs.hpp"

using SubmitAndWaitHandler = std::function<void(RequestType, CommunicationChannel *)>;

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
struct OpenatAwaitable;
struct StatxAwaitable;
struct UnlinkatAwaitable;
struct RenameatAwaitable;

struct GenericResponse {
  CommunicationChannel *channel{};
  bool await_ready() const noexcept { return false; }
  void await_suspend(EvTask::Handle h) { channel = &h.promise().state.com_data; }
  CommunicationChannel *await_resume() { return channel; }
};

class EventManager {
  enum LivingState { NOT_STARTED, LIVING, DYING, DEAD };
  LivingState manager_life_state_{};

  static std::mutex init_mutex;
  static int shared_ring_fd;
  static int ring_instances;

  io_uring ring{};

  std::vector<EvTask *> coroutines_to_start{};

  void await_message();
  void event_handler(int res, RequestData *req_data);

  std::size_t in_flight_requests{};
  bool should_restrict_usage();
  EvTask kill_coro_task;
  EvTask kill_internal();

public:
  EvTask kill();

  // registering the coroutine puts it in a vector, the vector is looped through
  // in the event loop and emptied, and each coroutine has start() called on
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
  OpenatAwaitable openat(int dirfd, const char *pathname, int flags, mode_t mode);
  StatxAwaitable statx(int dirfd, const char *pathname, int flags, unsigned int mask, struct statx *statxbuf);
  UnlinkatAwaitable unlinkat(int dirfd, const char *pathname, int flags);
  RenameatAwaitable renameat(int olddirfd, const char *oldpathname, int newdirfd, const char *newpathname,
                             int flags);

  RequestQueue make_request_queue();
  EvTask submit_and_wait(const RequestQueue &requests_vec, SubmitAndWaitHandler handler);
};

#endif