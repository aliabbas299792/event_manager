#ifndef IO_AWAITABLES_
#define IO_AWAITABLES_

#include <bits/types/struct_iovec.h>
#include <liburing.h>
#include <sys/socket.h>

#include "communication/communication_channel.hpp"
#include "communication/communication_types.hpp"
#include "event_loop/event_manager.hpp"
#include "event_loop/request_data.hpp"
#include "task.hpp"

/*
Each awaitable must implement `void prepare_sqring_op(EvTask::Handle handle)`
CRTP is used to call them from the IOAwaitable, as this can significantly reduce
code duplication
*/

enum class EventSystemError : char {
  NO_ERROR = 0,
  SUBMISSION_QUEUE_FULL = -1,
  SUBMIT_ERROR = -2,
  SYSTEM_COMMUNICATION_CHANNEL_FAILURE = -3
};

template <typename Data> struct IOResponse {
  // event system errors include io_uring errors as well as errors for the
  // EventManager, like not being able to find data that was thought to have
  // been stored in the communication channel
  EventSystemError event_system_error{};
  int error_num{};
  Data data{};
};

template <RequestType Rt, typename DerivedAwaitable> struct IOAwaitable {
  CommunicationChannel *channel{};
  RequestData req_data{};

  EventSystemError error_code{}; // return value of the function
  int error_num{};               // errno
  EventManager *const ev;
  io_uring_sqe *const sqe;

  bool await_ready() const noexcept {
    // if the initial return code is non zero then we have run into an error
    // and cannot proceed
    return static_cast<int>(error_code) != 0;
  }

  void await_suspend(EvTask::Handle handle) {
    channel = &handle.promise().state.com_data;
    req_data.handle = handle; // just got the handle, so set it

    static_cast<DerivedAwaitable *>(this)->prepare_sqring_op(handle, sqe);
    io_uring_sqe_set_data(sqe, &req_data);

    auto ret = ev->submit_queued_entries();
    if (ret < 1) { // since submit returns the number of entries submitted
      std::cerr << "io_uring_submit failed\n";
      error_code = EventSystemError::SUBMIT_ERROR;
      error_num = -ret;
      handle.resume();
    }
  }

  IOResponse<RespDataTypeMap<Rt>> await_resume() {
    if (static_cast<int>(error_code) != 0) {
      return {.event_system_error = error_code, .error_num = error_num};
    }

    auto val = channel->consume_resp_data<Rt>();
    if (!val.has_value()) {
      return {.event_system_error = EventSystemError::SYSTEM_COMMUNICATION_CHANNEL_FAILURE,
              .error_num = error_num};
    }
    return {.data = val.value()};
  }

  IOAwaitable(EventManager *ev) : ev(ev), sqe(ev->get_uring_sqe()) {
    if (sqe == nullptr) {
      error_code = EventSystemError::SUBMISSION_QUEUE_FULL;
    }

    req_data.req_type = Rt;
  }
};

struct ReadAwaitable : IOAwaitable<RequestType::READ, ReadAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &read_data = req_data.specific_data.read_data;
    io_uring_prep_read(sqe, read_data.fd, read_data.buffer, read_data.length, 0);
  }

  ReadAwaitable(int fd, uint8_t *buff, size_t length, EventManager *ev) : IOAwaitable(ev) {
    auto &read_data = req_data.specific_data.read_data;
    read_data = {fd, buff, length};
  }

  // default initialiser
  ReadAwaitable() : IOAwaitable(nullptr) {}
};

struct WriteAwaitable : IOAwaitable<RequestType::WRITE, WriteAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &write_data = req_data.specific_data.write_data;
    io_uring_prep_write(sqe, write_data.fd, write_data.buffer, write_data.length, 0);
  }

  WriteAwaitable(int fd, const uint8_t *buff, size_t length, EventManager *ev) : IOAwaitable(ev) {
    auto &write_data = req_data.specific_data.write_data;
    write_data = {fd, buff, length};
  }

  // default initialiser
  WriteAwaitable() : IOAwaitable(nullptr) {}
};

struct CloseAwaitable : IOAwaitable<RequestType::CLOSE, CloseAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &close_data = req_data.specific_data.close_data;
    io_uring_prep_close(sqe, close_data.fd);
  }

  CloseAwaitable(int fd, EventManager *ev) : IOAwaitable(ev) {
    auto &close_data = req_data.specific_data.close_data;
    close_data = {fd};
  }

  // default initialiser
  CloseAwaitable() : IOAwaitable(nullptr) {}
};

struct ShutdownAwaitable : IOAwaitable<RequestType::SHUTDOWN, ShutdownAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &shutdown_data = req_data.specific_data.shutdown_data;
    io_uring_prep_shutdown(sqe, shutdown_data.fd, shutdown_data.how);
  }

  ShutdownAwaitable(int fd, int how, EventManager *ev) : IOAwaitable(ev) {
    auto &shutdown_data = req_data.specific_data.shutdown_data;
    shutdown_data = {fd, how};
  }

  // default initialiser
  ShutdownAwaitable() : IOAwaitable(nullptr) {}
};

struct ReadvAwaitable : IOAwaitable<RequestType::READV, ReadvAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &readv_data = req_data.specific_data.readv_data;
    io_uring_prep_readv(sqe, readv_data.fd, readv_data.iovs, readv_data.num, 0);
  }

  ReadvAwaitable(int fd, struct iovec *iovs, size_t num, EventManager *ev) : IOAwaitable(ev) {
    auto &readv_data = req_data.specific_data.readv_data;
    readv_data = {fd, iovs, num};
  }

  // default initialiser
  ReadvAwaitable() : IOAwaitable(nullptr) {}
};

struct WritevAwaitable : IOAwaitable<RequestType::WRITEV, WritevAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &writev_data = req_data.specific_data.writev_data;
    io_uring_prep_writev(sqe, writev_data.fd, writev_data.iovs, writev_data.num, 0);
  }

  WritevAwaitable(int fd, struct iovec *iovs, size_t num, EventManager *ev) : IOAwaitable(ev) {
    auto &writev_data = req_data.specific_data.writev_data;
    writev_data = {fd, iovs, num};
  }

  // default initialiser
  WritevAwaitable() : IOAwaitable(nullptr) {}
};

struct AcceptAwaitable : IOAwaitable<RequestType::ACCEPT, AcceptAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &accept_data = req_data.specific_data.accept_data;
    io_uring_prep_accept(sqe, accept_data.sockfd, accept_data.addr, accept_data.addrlen, 0);
  }

  AcceptAwaitable(int sockfd, sockaddr *addr, socklen_t *addrlen, EventManager *ev) : IOAwaitable(ev) {
    auto &accept_data = req_data.specific_data.accept_data;
    accept_data = {sockfd, addr, addrlen};
  }

  // default initialiser
  AcceptAwaitable() : IOAwaitable(nullptr) {}
};

struct ConnectAwaitable : IOAwaitable<RequestType::CONNECT, ConnectAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &connect_data = req_data.specific_data.connect_data;
    io_uring_prep_connect(sqe, connect_data.sockfd, connect_data.addr, connect_data.addrlen);
  }

  ConnectAwaitable(int sockfd, const sockaddr *addr, socklen_t addrlen, EventManager *ev) : IOAwaitable(ev) {
    auto &connect_data = req_data.specific_data.connect_data;
    connect_data = {sockfd, addr, addrlen};
  }

  // default initialiser
  ConnectAwaitable() : IOAwaitable(nullptr) {}
};

struct OpenatAwaitable : IOAwaitable<RequestType::OPENAT, OpenatAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &openat_data = req_data.specific_data.openat_data;
    io_uring_prep_openat(sqe, openat_data.dirfd, openat_data.pathname, openat_data.flags, openat_data.mode);
  }

  OpenatAwaitable(int dirfd, const char *pathname, int flags, mode_t mode, EventManager *ev)
      : IOAwaitable(ev) {
    auto &openat_data = req_data.specific_data.openat_data;
    openat_data = {dirfd, pathname, flags, mode};
  }

  // default initialiser
  OpenatAwaitable() : IOAwaitable(nullptr) {}
};

struct StatxAwaitable : IOAwaitable<RequestType::STATX, StatxAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &statx_data = req_data.specific_data.statx_data;
    io_uring_prep_statx(sqe, statx_data.dirfd, statx_data.pathname, statx_data.flags, statx_data.mask,
                        statx_data.statxbuf);
  }

  StatxAwaitable(int dirfd, const char *pathname, int flags, unsigned int mask, struct statx *statxbuf,
                 EventManager *ev)
      : IOAwaitable(ev) {
    auto &statx_data = req_data.specific_data.statx_data;
    statx_data = {dirfd, pathname, flags, mask, statxbuf};
  }

  // default initialiser
  StatxAwaitable() : IOAwaitable(nullptr) {}
};

struct UnlinkatAwaitable : IOAwaitable<RequestType::UNLINKAT, UnlinkatAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &unlinkat_data = req_data.specific_data.unlinkat_data;
    io_uring_prep_unlinkat(sqe, unlinkat_data.dirfd, unlinkat_data.pathname, unlinkat_data.flags);
  }

  UnlinkatAwaitable(int dirfd, const char *pathname, int flags, EventManager *ev) : IOAwaitable(ev) {
    auto &unlinkat_data = req_data.specific_data.unlinkat_data;
    unlinkat_data = {dirfd, pathname, flags};
  }

  // default initialiser
  UnlinkatAwaitable() : IOAwaitable(nullptr) {}
};

struct RenameatAwaitable : IOAwaitable<RequestType::RENAMEAT, RenameatAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &renameat_data = req_data.specific_data.renameat_data;
    io_uring_prep_renameat(sqe, renameat_data.olddirfd, renameat_data.oldpathname, renameat_data.newdirfd,
                           renameat_data.newpathname, renameat_data.flags);
  }

  RenameatAwaitable(int olddirfd, const char *oldpathname, int newdirfd, const char *newpathname, int flags,
                    EventManager *ev)
      : IOAwaitable(ev) {
    auto &renameat_data = req_data.specific_data.renameat_data;
    renameat_data = {olddirfd, oldpathname, newdirfd, newpathname, flags};
  }

  // default initialiser
  RenameatAwaitable() : IOAwaitable(nullptr) {}
};

#endif