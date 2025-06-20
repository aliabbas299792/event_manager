#include "communication/communication_types.hpp"
#include "coroutine/io_awaitables.hpp"
#include "coroutine/task.hpp"
#include "errors.hpp"
#include "event_loop/parameter_packs.hpp"
#include "event_loop/request_data.hpp"
#include "event_manager.hpp"
#include <cstddef>
#include <liburing.h>
#include <liburing/io_uring.h>

using namespace ErrorProcessing;

struct RetrieveCurrentHandle {
  EvTask::Handle handle;
  bool await_ready() noexcept {
    return false;
  }
  void await_suspend(EvTask::Handle h) noexcept {
    handle = h;
    handle.resume();
  }
  EvTask::Handle await_resume() noexcept {
    return handle;
  }
};

ReadAwaitable EventManager::read(int fd, uint8_t* buffer, size_t length) {
  if (should_restrict_usage())
    return {};
  return ReadAwaitable{fd, buffer, length, this};
}

WriteAwaitable EventManager::write(int fd, const uint8_t* buffer, size_t length) {
  if (should_restrict_usage())
    return {};
  return WriteAwaitable{fd, buffer, length, this};
}

CloseAwaitable EventManager::close(int fd) {
  if (should_restrict_usage())
    return {};
  return CloseAwaitable{fd, this};
}

ShutdownAwaitable EventManager::shutdown(int fd, int how) {
  if (should_restrict_usage())
    return {};
  return ShutdownAwaitable{fd, how, this};
}

ReadvAwaitable EventManager::readv(int fd, struct iovec* iovs, size_t num) {
  if (should_restrict_usage())
    return {};
  return ReadvAwaitable{fd, iovs, num, this};
}

WritevAwaitable EventManager::writev(int fd, struct iovec* iovs, size_t num) {
  if (should_restrict_usage())
    return {};
  return WritevAwaitable{fd, iovs, num, this};
}

AcceptAwaitable EventManager::accept(int sockfd, sockaddr* addr, socklen_t* addrlen) {
  if (should_restrict_usage())
    return {};
  return AcceptAwaitable{sockfd, addr, addrlen, this};
}

ConnectAwaitable EventManager::connect(int sockfd, const sockaddr* addr, socklen_t addrlen) {
  if (should_restrict_usage())
    return {};
  return ConnectAwaitable{sockfd, addr, addrlen, this};
}

OpenatAwaitable EventManager::openat(int dirfd, const char* pathname, int flags, mode_t mode) {
  if (should_restrict_usage())
    return {};
  return OpenatAwaitable{dirfd, pathname, flags, mode, this};
}
StatxAwaitable EventManager::statx(int dirfd, const char* pathname, int flags, unsigned int mask,
                                   struct statx* statxbuf) {
  if (should_restrict_usage())
    return {};
  return StatxAwaitable{dirfd, pathname, flags, mask, statxbuf, this};
}
UnlinkatAwaitable EventManager::unlinkat(int dirfd, const char* pathname, int flags) {
  if (should_restrict_usage())
    return {};
  return UnlinkatAwaitable{dirfd, pathname, flags, this};
}
RenameatAwaitable EventManager::renameat(int olddirfd, const char* oldpathname, int newdirfd,
                                         const char* newpathname, int flags) {
  if (should_restrict_usage())
    return {};
  return RenameatAwaitable{olddirfd, oldpathname, newdirfd, newpathname, flags, this};
}

RequestQueue EventManager::make_request_queue() {
  return RequestQueue{};
}

bool EventManager::process_single_generic_request(const OperationParameterPackVariant& req,
                                                  RequestData& single_req, EvTask::Handle handle) {
  auto req_type = static_cast<RequestType>(req.index());

  auto sqe = get_uring_sqe();

  if (sqe == nullptr) {
    std::cerr << "There was an error in making a request (event manager "
                 "system error for queueing)\n";
    return false;
  }

  single_req.handle = handle;
  single_req.req_type = req_type;
  single_req.allocated_dynamic = false;
  auto& specific_data = single_req.specific_data;

  switch (req_type) {
  case RequestType::READ: {
    auto* pack = std::get_if<ReadParameterPack>(&req);
    if (pack) {
      specific_data.read_data = {pack->fd, pack->buffer, pack->length};
      io_uring_prep_read(sqe, pack->fd, pack->buffer, pack->length, 0);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  case RequestType::WRITE: {
    auto* pack = std::get_if<WriteParameterPack>(&req);
    if (pack) {
      specific_data.write_data = {pack->fd, pack->buffer, pack->length};
      io_uring_prep_write(sqe, pack->fd, pack->buffer, pack->length, 0);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  case RequestType::CLOSE: {
    auto* pack = std::get_if<CloseParameterPack>(&req);
    if (pack) {
      specific_data.close_data = {pack->fd};
      io_uring_prep_close(sqe, pack->fd);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  case RequestType::SHUTDOWN: {
    auto* pack = std::get_if<ShutdownParameterPack>(&req);
    if (pack) {
      specific_data.shutdown_data = {pack->fd, pack->how};
      io_uring_prep_shutdown(sqe, pack->fd, pack->how);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  case RequestType::READV: {
    auto* pack = std::get_if<ReadvParameterPack>(&req);
    if (pack) {
      specific_data.readv_data = {pack->fd, pack->iovs, pack->num};
      io_uring_prep_readv(sqe, pack->fd, pack->iovs, pack->num, 0);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  case RequestType::WRITEV: {
    auto* pack = std::get_if<WritevParameterPack>(&req);
    if (pack) {
      specific_data.writev_data = {pack->fd, pack->iovs, pack->num};
      io_uring_prep_writev(sqe, pack->fd, pack->iovs, pack->num, 0);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  case RequestType::ACCEPT: {
    auto* pack = std::get_if<AcceptParameterPack>(&req);
    if (pack) {
      specific_data.accept_data = {pack->sockfd, pack->addr, pack->addrlen};
      io_uring_prep_accept(sqe, pack->sockfd, pack->addr, pack->addrlen, 0);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  case RequestType::CONNECT: {
    auto* pack = std::get_if<ConnectParameterPack>(&req);
    if (pack) {
      specific_data.connect_data = {pack->sockfd, pack->addr, pack->addrlen};
      io_uring_prep_connect(sqe, pack->sockfd, pack->addr, pack->addrlen);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  case RequestType::OPENAT: {
    auto* pack = std::get_if<OpenatParameterPack>(&req);
    if (pack) {
      specific_data.openat_data = {pack->dirfd, pack->pathname, pack->flags, pack->mode};
      io_uring_prep_openat(sqe, pack->dirfd, pack->pathname, pack->flags, pack->mode);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  case RequestType::STATX: {
    auto* pack = std::get_if<StatxParameterPack>(&req);
    if (pack) {
      specific_data.statx_data = {pack->dirfd, pack->pathname, pack->flags, pack->mask, pack->statxbuf};
      io_uring_prep_statx(sqe, pack->dirfd, pack->pathname, pack->flags, pack->mask, pack->statxbuf);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  case RequestType::UNLINKAT: {
    auto* pack = std::get_if<UnlinkatParameterPack>(&req);
    if (pack) {
      specific_data.unlinkat_data = {pack->dirfd, pack->pathname, pack->flags};
      io_uring_prep_unlinkat(sqe, pack->dirfd, pack->pathname, pack->flags);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  case RequestType::RENAMEAT: {
    auto* pack = std::get_if<RenameatParameterPack>(&req);
    if (pack) {
      specific_data.renameat_data = {pack->olddirfd, pack->oldpathname, pack->newdirfd, pack->newpathname,
                                     pack->flags};
      io_uring_prep_renameat(sqe, pack->olddirfd, pack->oldpathname, pack->newdirfd, pack->newpathname,
                             pack->flags);
    } else {
      std::cerr << "There was an error in retrieving queued data\n";
      return false;
    }
    break;
  }
  }

  io_uring_sqe_set_data(sqe, &single_req);

  return true;
}

EvTask EventManager::submit_and_wait(const RequestQueue& request_queue, SubmitAndWaitHandler handler) {
  auto& requests_vec = request_queue.req_vec;

  auto ret = submit_queued_entries();
  if (ret < 0) {
    co_return ret;
  }

  std::vector<RequestData> req_data{requests_vec.size()};
  auto handle = co_await RetrieveCurrentHandle{};

  for (std::size_t i = 0; i < requests_vec.size(); i++) {
    auto& req = requests_vec[i];
    auto& single_req = req_data[i];
    process_single_generic_request(req, single_req, handle);
  }

  // how many are currently being processed
  size_t num_submitted = ret;

  // how many are still queued for submission
  auto num_queued = _ring.sq.sqe_tail - _ring.sq.sqe_head;

  while (num_submitted != 0 || num_queued != 0) {
    while (num_queued != 0 && num_submitted == 0) {
      auto ret = submit_queued_entries();
      if (ret < 0) {
        co_return ret;
      }

      num_submitted += ret;
      num_queued -= ret;
    }

    for (size_t i = 0; i < num_submitted; i++) {
      auto channel = co_await GenericResponse{};
      auto response_type = channel->response_store_current_type();
      handler(response_type, channel);
    }

    num_submitted = 0;
  }

  co_return 0;
}

Errnos EventManager::submit_request(io_uring_sqe* sqe, RequestData* req_data) {
  io_uring_sqe_set_data(sqe, req_data);

  auto ret = submit_queued_entries();
  if (ret < 1) {  // since submit returns the number of entries submitted
    std::cerr << "io_uring_submit failed\n";
    return static_cast<Errnos>(-ret);
  }

  return Errnos::UNKNOWN_ERROR;  // no error == unknown error in this context
}

std::pair<io_uring_sqe*, RequestData*> EventManager::get_sqe_and_req_data(RequestType req_type) {
  auto req_data = new RequestData{};
  req_data->req_type = req_type;
  req_data->allocated_dynamic = true;

  auto sqe = get_uring_sqe();
  return {sqe, req_data};
}

Errnos EventManager::read_na(int fd, uint8_t* buffer, size_t length) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::READ);

  auto& read_data = req_data->specific_data.read_data;
  read_data = {fd, buffer, length};
  io_uring_prep_read(sqe, fd, buffer, length, 0);

  return submit_request(sqe, req_data);
}

Errnos EventManager::write_na(int fd, const uint8_t* buffer, size_t length) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::WRITE);

  auto& write_data = req_data->specific_data.write_data;
  write_data = {fd, buffer, length};
  io_uring_prep_write(sqe, write_data.fd, write_data.buffer, write_data.length, 0);

  return submit_request(sqe, req_data);
}

Errnos EventManager::close_na(int fd) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::CLOSE);

  auto& close_data = req_data->specific_data.close_data;
  close_data = {fd};
  io_uring_prep_close(sqe, close_data.fd);

  return submit_request(sqe, req_data);
}

Errnos EventManager::shutdown_na(int fd, int how) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::SHUTDOWN);

  auto& shutdown_data = req_data->specific_data.shutdown_data;
  shutdown_data = {fd, how};
  io_uring_prep_shutdown(sqe, shutdown_data.fd, shutdown_data.how);

  return submit_request(sqe, req_data);
}

Errnos EventManager::readv_na(int fd, struct iovec* iovs, size_t num) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::READV);

  auto& readv_data = req_data->specific_data.readv_data;
  readv_data = {fd, iovs, num};
  io_uring_prep_readv(sqe, readv_data.fd, readv_data.iovs, readv_data.num, 0);

  return submit_request(sqe, req_data);
}

Errnos EventManager::writev_na(int fd, struct iovec* iovs, size_t num) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::WRITEV);

  auto& writev_data = req_data->specific_data.writev_data;
  writev_data = {fd, iovs, num};
  io_uring_prep_writev(sqe, writev_data.fd, writev_data.iovs, writev_data.num, 0);

  return submit_request(sqe, req_data);
}

Errnos EventManager::accept_na(int sockfd, sockaddr* addr, socklen_t* addrlen) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::ACCEPT);

  auto& accept_data = req_data->specific_data.accept_data;
  accept_data = {sockfd, addr, addrlen};
  io_uring_prep_accept(sqe, accept_data.sockfd, accept_data.addr, accept_data.addrlen, 0);

  return submit_request(sqe, req_data);
}

Errnos EventManager::connect_na(int sockfd, const sockaddr* addr, socklen_t addrlen) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::CONNECT);

  auto& connect_data = req_data->specific_data.connect_data;
  connect_data = {sockfd, addr, addrlen};
  io_uring_prep_connect(sqe, connect_data.sockfd, connect_data.addr, connect_data.addrlen);

  return submit_request(sqe, req_data);
}

Errnos EventManager::openat_na(int dirfd, const char* pathname, int flags, mode_t mode) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::OPENAT);

  auto& openat_data = req_data->specific_data.openat_data;
  openat_data = {dirfd, pathname, flags, mode};
  io_uring_prep_openat(sqe, openat_data.dirfd, openat_data.pathname, openat_data.flags, openat_data.mode);

  return submit_request(sqe, req_data);
}

Errnos EventManager::statx_na(int dirfd, const char* pathname, int flags, unsigned int mask,
                              struct statx* statxbuf) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::STATX);

  auto& statx_data = req_data->specific_data.statx_data;
  statx_data = {dirfd, pathname, flags, mask, statxbuf};
  io_uring_prep_statx(sqe, statx_data.dirfd, statx_data.pathname, statx_data.flags, statx_data.mask,
                      statx_data.statxbuf);

  return submit_request(sqe, req_data);
}

Errnos EventManager::unlinkat_na(int dirfd, const char* pathname, int flags) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::UNLINKAT);

  auto& unlinkat_data = req_data->specific_data.unlinkat_data;
  unlinkat_data = {dirfd, pathname, flags};
  io_uring_prep_unlinkat(sqe, unlinkat_data.dirfd, unlinkat_data.pathname, unlinkat_data.flags);

  return submit_request(sqe, req_data);
}

Errnos EventManager::renameat_na(int olddirfd, const char* oldpathname, int newdirfd, const char* newpathname,
                                 int flags) {
  auto [sqe, req_data] = get_sqe_and_req_data(RequestType::RENAMEAT);

  auto& renameat_data = req_data->specific_data.renameat_data;
  renameat_data = {olddirfd, oldpathname, newdirfd, newpathname, flags};
  io_uring_prep_renameat(sqe, renameat_data.olddirfd, renameat_data.oldpathname, renameat_data.newdirfd,
                         renameat_data.newpathname, renameat_data.flags);

  return submit_request(sqe, req_data);
}

EvTask EventManager::poll(PollHandler handler) {
  if (_polling_requests) {
    co_return -1;
  }

  _polling_requests = true;
  PollingState continue_polling = PollingState::CONTINUE_POLLING;
  while (continue_polling == PollingState::CONTINUE_POLLING) {
    _polling_handle = co_await RetrieveCurrentHandle{};
    auto channel = co_await GenericResponse{};
    _polling_handle = nullptr;

    auto response_type = channel->response_store_current_type();
    continue_polling = handler(this, response_type, channel);
  }
  _polling_requests = false;

  co_return 0;
}
