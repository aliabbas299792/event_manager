#ifndef IO_AWAITABLES_
#define IO_AWAITABLES_

#include <bits/types/struct_iovec.h>
#include <liburing.h>
#include <sys/socket.h>

#include "communication/communication_channel.hpp"
#include "communication/communication_types.hpp"
#include "event_loop/request_data.hpp"
#include "task.hpp"

/*
Each awaitable must implement `void prepare_sqring_op(EvTask::Handle handle)`
CRTP is used to call them from the IOAwaitable, as this can significantly reduce
code duplication
*/

template <typename Data> struct IOResponse {
  int initial_error{};
  Data data{};
};

template <RequestType Rt, typename DerivedAwaitable> struct IOAwaitable {
  CommunicationChannel *channel{};
  RequestData req_data{};

  int error_code{};
  io_uring *const ring;
  io_uring_sqe *const sqe;

  bool await_ready() const noexcept {
    // if the initial return code is non zero then we have run into an error
    // and cannot proceed
    return error_code != 0;
  }

  void await_suspend(EvTask::Handle handle) {
    channel = &handle.promise().state.com_data;
    req_data.handle = handle; // just got the handle, so set it

    static_cast<DerivedAwaitable *>(this)->prepare_sqring_op(handle, sqe);
    io_uring_sqe_set_data(sqe, &req_data);

    auto ret = io_uring_submit(ring);
    if (ret < 1) { // since submit returns the number of entries submitted
      std::cerr << "io_uring_submit failed\n";
      error_code = ret;
      handle.resume();
    }
  }

  IOResponse<RespDataTypeMap<Rt>> await_resume() {
    if (error_code != 0) {
      return {.initial_error = error_code};
    }

    auto val = channel->get_resp_data<Rt>();
    return {.data = val.value()};
  }

  IOAwaitable(io_uring *ring) : ring(ring), sqe(io_uring_get_sqe(ring)) {
    if (sqe == nullptr) {
      error_code = -1;
    }

    req_data.req_type = Rt;
  }
};

struct ReadAwaitable : IOAwaitable<RequestType::READ, ReadAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &read_data = req_data.specific_data.read_data;
    io_uring_prep_read(sqe, read_data.fd, read_data.buffer, read_data.length,
                       0);
  }

  ReadAwaitable(int fd, uint8_t *buff, size_t length, io_uring *ring)
      : IOAwaitable(ring) {
    auto &read_data = req_data.specific_data.read_data;
    read_data = {fd, buff, length};
  }
};

struct WriteAwaitable : IOAwaitable<RequestType::WRITE, WriteAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &write_data = req_data.specific_data.write_data;
    io_uring_prep_write(sqe, write_data.fd, write_data.buffer,
                        write_data.length, 0);
  }

  WriteAwaitable(int fd, uint8_t *buff, size_t length, io_uring *ring)
      : IOAwaitable(ring) {
    auto &write_data = req_data.specific_data.write_data;
    write_data = {fd, buff, length};
  }
};

struct CloseAwaitable : IOAwaitable<RequestType::CLOSE, CloseAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &close_data = req_data.specific_data.close_data;
    io_uring_prep_close(sqe, close_data.fd);
  }

  CloseAwaitable(int fd, io_uring *ring) : IOAwaitable(ring) {
    auto &close_data = req_data.specific_data.close_data;
    close_data = {fd};
  }
};

struct ShutdownAwaitable
    : IOAwaitable<RequestType::SHUTDOWN, ShutdownAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &shutdown_data = req_data.specific_data.shutdown_data;
    io_uring_prep_shutdown(sqe, shutdown_data.fd, shutdown_data.how);
  }

  ShutdownAwaitable(int fd, int how, io_uring *ring) : IOAwaitable(ring) {
    auto &shutdown_data = req_data.specific_data.shutdown_data;
    shutdown_data = {fd, how};
  }
};

struct ReadvAwaitable : IOAwaitable<RequestType::READV, ReadvAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &readv_data = req_data.specific_data.readv_data;
    io_uring_prep_readv(sqe, readv_data.fd, readv_data.iovs, readv_data.num, 0);
  }

  ReadvAwaitable(int fd, struct iovec *iovs, size_t num, io_uring *ring)
      : IOAwaitable(ring) {
    auto &readv_data = req_data.specific_data.readv_data;
    readv_data = {fd, iovs, num};
  }
};

struct WritevAwaitable : IOAwaitable<RequestType::WRITEV, WritevAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &writev_data = req_data.specific_data.writev_data;
    io_uring_prep_writev(sqe, writev_data.fd, writev_data.iovs, writev_data.num,
                         0);
  }

  WritevAwaitable(int fd, struct iovec *iovs, size_t num, io_uring *ring)
      : IOAwaitable(ring) {
    auto &writev_data = req_data.specific_data.writev_data;
    writev_data = {fd, iovs, num};
  }
};

struct AcceptAwaitable : IOAwaitable<RequestType::ACCEPT, AcceptAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &accept_data = req_data.specific_data.accept_data;
    io_uring_prep_accept(sqe, accept_data.sockfd, accept_data.addr,
                         accept_data.addrlen, 0);
  }

  AcceptAwaitable(int sockfd, sockaddr *addr, socklen_t *addrlen,
                  io_uring *ring)
      : IOAwaitable(ring) {
    auto &accept_data = req_data.specific_data.accept_data;
    accept_data = {sockfd, addr, addrlen};
  }
};

struct ConnectAwaitable : IOAwaitable<RequestType::CONNECT, ConnectAwaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &connect_data = req_data.specific_data.connect_data;
    io_uring_prep_connect(sqe, connect_data.sockfd, connect_data.addr,
                          connect_data.addrlen);
  }

  ConnectAwaitable(int sockfd, const sockaddr *addr, socklen_t addrlen,
                   io_uring *ring)
      : IOAwaitable(ring) {
    auto &connect_data = req_data.specific_data.connect_data;
    connect_data = {sockfd, addr, addrlen};
  }
};

#endif