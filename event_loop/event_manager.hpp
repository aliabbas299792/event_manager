#ifndef EVENT_MANAGER_
#define EVENT_MANAGER_

#include <cstddef>
#include <functional>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>
#include <unordered_set>
#include <vector>

#include "communication/communication_channel.hpp"
#include "communication/communication_types.hpp"
#include "coroutine/task.hpp"
#include "errors.hpp"
#include "event_loop/request_data.hpp"
#include "parameter_packs.hpp"

template <typename T>
class ItemStore {
  std::vector<T> _items;
  std::unordered_set<size_t> _freed_idxs{};

public:
  ItemStore() : _items{} {};

  size_t insert(T&& item) {
    size_t selected_idx = 0;
    if (_freed_idxs.size() != 0) {
      selected_idx = *_freed_idxs.begin();
      _freed_idxs.erase(selected_idx);
      _items[selected_idx] = std::move(item);
    } else {
      _items.push_back(std::move(item));
      selected_idx = _items.size() - 1;
    }
    return selected_idx;
  }

  T& operator[](size_t idx) {
    return _items[idx];
  }

  void erase(size_t idx) {
    _freed_idxs.insert(idx);
  }
};

class EventManager;
enum class PollingState { CONTINUE_POLLING, STOP_POLLING };

using SubmitAndWaitHandler = std::function<void(RequestType, CommunicationChannel*)>;
using PollHandler = std::function<PollingState(EventManager*, RequestType, CommunicationChannel*)>;

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
  CommunicationChannel* channel{};
  bool await_ready() const noexcept {
    return false;
  }
  void await_suspend(EvTask::Handle h) {
    channel = &h.promise().state.com_data;
  }
  CommunicationChannel* await_resume() {
    return channel;
  }
};

class EventManager {
  enum LivingState { NOT_STARTED, LIVING, DYING, DEAD };
  LivingState _manager_life_state{};

  static std::mutex init_mutex;
  static int shared_ring_fd;
  static size_t ring_instances;

  io_uring _ring{};

  ItemStore<EvTask> _managed_coroutines_store{};

  bool _polling_requests{};  // to prevent trying to poll within the polling context
  EvTask::Handle _polling_handle = nullptr;
  std::vector<std::pair<int, RequestData*>> _ready_requests_store{};

  void await_message();
  void event_handler(int res, RequestData* req_data);

  std::size_t _in_flight_requests{};
  bool should_restrict_usage();
  EvTask _kill_coro_task;
  EvTask kill_internal();

  Errnos submit_request(io_uring_sqe* sqe, RequestData* req_data);
  std::pair<io_uring_sqe*, RequestData*> get_sqe_and_req_data(RequestType req_type);
  bool process_single_generic_request(const OperationParameterPackVariant& req, RequestData& single_req,
                                      EvTask::Handle handle);

public:
  EvTask kill();

  // register a coroutine by passing in the coroutine function and its parameters
  template <typename CoroFn, typename... Args>
  void register_coro(CoroFn fn, Args&&... args) {
    auto coro = fn(std::forward<Args>(args)...);
    this->register_coro(std::move(coro));
  }

  // register a coroutine by moving in a previously constructed coroutine
  void register_coro(EvTask&& coro);

  EventManager(size_t queue_depth);

  void start();
  int submit_queued_entries();
  io_uring_sqe* get_uring_sqe();

  [[nodiscard]] ReadAwaitable read(int fd, uint8_t* buffer, size_t length);
  [[nodiscard]] WriteAwaitable write(int fd, const uint8_t* buffer, size_t length);
  [[nodiscard]] CloseAwaitable close(int fd);
  [[nodiscard]] ShutdownAwaitable shutdown(int fd, int how);
  [[nodiscard]] ReadvAwaitable readv(int fd, struct iovec* iovs, size_t num);
  [[nodiscard]] WritevAwaitable writev(int fd, struct iovec* iovs, size_t num);
  [[nodiscard]] AcceptAwaitable accept(int sockfd, sockaddr* addr, socklen_t* addrlen);
  [[nodiscard]] ConnectAwaitable connect(int sockfd, const sockaddr* addr, socklen_t addrlen);
  [[nodiscard]] OpenatAwaitable openat(int dirfd, const char* pathname, int flags, mode_t mode);
  [[nodiscard]] StatxAwaitable statx(int dirfd, const char* pathname, int flags, unsigned int mask,
                                     struct statx* statxbuf);
  [[nodiscard]] UnlinkatAwaitable unlinkat(int dirfd, const char* pathname, int flags);
  [[nodiscard]] RenameatAwaitable renameat(int olddirfd, const char* oldpathname, int newdirfd,
                                           const char* newpathname, int flags);

  // non awaitable versions of the above functions so that they can be polled instead (_na = non awaitable)
  Errnos read_na(int fd, uint8_t* buffer, size_t length);
  Errnos write_na(int fd, const uint8_t* buffer, size_t length);
  Errnos close_na(int fd);
  Errnos shutdown_na(int fd, int how);
  Errnos readv_na(int fd, struct iovec* iovs, size_t num);
  Errnos writev_na(int fd, struct iovec* iovs, size_t num);
  Errnos accept_na(int sockfd, sockaddr* addr, socklen_t* addrlen);
  Errnos connect_na(int sockfd, const sockaddr* addr, socklen_t addrlen);
  Errnos openat_na(int dirfd, const char* pathname, int flags, mode_t mode);
  Errnos statx_na(int dirfd, const char* pathname, int flags, unsigned int mask, struct statx* statxbuf);
  Errnos unlinkat_na(int dirfd, const char* pathname, int flags);
  Errnos renameat_na(int olddirfd, const char* oldpathname, int newdirfd, const char* newpathname, int flags);
  EvTask poll(PollHandler handler);

  // for batch submissions
  RequestQueue make_request_queue();
  EvTask submit_and_wait(const RequestQueue& requests_vec, SubmitAndWaitHandler handler);
};

#endif