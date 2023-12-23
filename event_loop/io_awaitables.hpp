#ifndef IO_AWAITABLES_
#define IO_AWAITABLES_

#include <bits/types/struct_iovec.h>
#include <liburing.h>
#include <sys/socket.h>

#include "communication/communication_channel.hpp"
#include "communication/response_types.hpp"
#include "coroutine/task.hpp"
#include "request_data.hpp"

/*
Each awaitable must implement `void suspend_action(EvTask::Handle handle)`
CRTP is used to call them from the IOAwaitable
I chose to do this so that all the awaitable lifecycle functions were in the parent
*/

template<typename Data>
struct IOResponse {
  int initial_error{};
  Data data{};
};

template <ResponseType Rt, typename DerivedAwaitable> struct IOAwaitable {
  CommunicationChannel *channel{};
  RequestData req_data{};
  EvTask::Handle  handle{};

  int error_code{};
  io_uring *const ring;
  io_uring_sqe *const sqe;

  bool await_ready() const noexcept {
    // std::cout << "checking await ready " << (error_code != 0) << "\n";
    // if the initial return code is non zero then we have run into an error
    // and cannot proceed
    return error_code != 0;
  }

  void await_suspend(EvTask::Handle handle) {
    // std::cout << "will suspend\n";
    this->handle = handle;
    static_cast<DerivedAwaitable*>(this)->suspend_action(handle);
  }

  IOResponse<RespTypeMap<Rt>> await_resume() {
    // std::cout << "await resume reached\n";
    if (error_code != 0) {
      return {.initial_error=error_code};
    }

    // std::cout << "reached the end of await resume\n";
    // std::cout << handle.done() << " (is it done?)\n";
    // std::cout << handle.resume() << " (is it done?)\n";
    // handle.resume();
    auto val = channel->get_resp_data<Rt>();
    // std::cout << "do we actually have a value: " << val.has_value() << "\n";
    return {.data=val.value()};
  }

  IOAwaitable(io_uring *ring) : ring(ring), sqe(io_uring_get_sqe(ring)) {
    if (sqe == nullptr) {
      error_code = -1;
    }
  }
};

struct ReadAwaitable : IOAwaitable<ResponseType::READ, ReadAwaitable> {
  void suspend_action(EvTask::Handle handle) {
    channel = &handle.promise().state.com_data;
    req_data.handle = handle; // just got the handle, so set it

    auto &read_data = req_data.specific_data.read_data;
    io_uring_prep_read(sqe, read_data.fd, read_data.buffer, read_data.length,
                       0);
    io_uring_sqe_set_data(sqe, &req_data);

    auto ret = io_uring_submit(ring);
    if (ret < 0) {
      std::cerr << "io_uring_submit failed\n";
      error_code = ret;
      handle.resume();
    }
    // std::cout << "at the end of the readawait suspend function\n";
  }

  ReadAwaitable(int fd, uint8_t *buff, size_t length, io_uring *ring)
      : IOAwaitable(ring) {
    req_data.req_type = ReqType::READ;

    auto &read_data = req_data.specific_data.read_data;
    read_data = {fd, buff, length};
  }
};

struct WriteAwaitable : IOAwaitable<ResponseType::WRITE, WriteAwaitable> {
  void suspend_action(EvTask::Handle handle) {
    channel = &handle.promise().state.com_data;
    req_data.handle = handle; // just got the handle, so set it

    auto &write_data = req_data.specific_data.write_data;
    io_uring_prep_write(sqe, write_data.fd, write_data.buffer,
                        write_data.length, 0);
    io_uring_sqe_set_data(sqe, &req_data);

    auto ret = io_uring_submit(ring);
    if (ret < 0) {
      std::cerr << "io_uring_submit failed\n";
      error_code = ret;
      handle.resume();
    }
  }

  WriteAwaitable(int fd, uint8_t *buff, size_t length, io_uring *ring)
      : IOAwaitable(ring) {
    req_data.req_type = ReqType::WRITE;

    auto &write_data = req_data.specific_data.write_data;
    write_data = {fd, buff, length};
  }
};

struct CloseAwaitable : IOAwaitable<ResponseType::CLOSE, CloseAwaitable> {
  void suspend_action(EvTask::Handle handle) {
    channel = &handle.promise().state.com_data;
    req_data.handle = handle; // just got the handle, so set it

    auto &close_data = req_data.specific_data.close_data;
    io_uring_prep_close(sqe, close_data.fd);
    io_uring_sqe_set_data(sqe, &req_data);

    auto ret = io_uring_submit(ring);
    if (ret < 0) {
      std::cerr << "io_uring_submit failed\n";
      error_code = ret;
      handle.resume();
    }
  }

  CloseAwaitable(int fd, io_uring *ring) : IOAwaitable(ring) {
    req_data.req_type = ReqType::CLOSE;

    auto &close_data = req_data.specific_data.close_data;
    close_data = {fd};
  }
};

struct ShutdownAwaitable : IOAwaitable<ResponseType::SHUTDOWN, ShutdownAwaitable> {
  void suspend_action(EvTask::Handle handle) {
    channel = &handle.promise().state.com_data;
    req_data.handle = handle; // just got the handle, so set it

    auto &shutdown_data = req_data.specific_data.shutdown_data;
    io_uring_prep_shutdown(sqe, shutdown_data.fd, shutdown_data.how);
    io_uring_sqe_set_data(sqe, &req_data);

    auto ret = io_uring_submit(ring);
    if (ret < 0) {
      std::cerr << "io_uring_submit failed\n";
      error_code = ret;
      handle.resume();
    }
  }

  ShutdownAwaitable(int fd, int how, io_uring *ring) : IOAwaitable(ring) {
    req_data.req_type = ReqType::SHUTDOWN;

    auto &shutdown_data = req_data.specific_data.shutdown_data;
    shutdown_data = {fd, how};
  }
};

struct ReadvAwaitable : IOAwaitable<ResponseType::READV, ReadvAwaitable> {
  void suspend_action(EvTask::Handle handle) {
    channel = &handle.promise().state.com_data;
    req_data.handle = handle; // just got the handle, so set it

    auto &readv_data = req_data.specific_data.readv_data;
    io_uring_prep_readv(sqe, readv_data.fd, readv_data.iovs, readv_data.num, 0);
    io_uring_sqe_set_data(sqe, &req_data);

    auto ret = io_uring_submit(ring);
    if (ret < 0) {
      std::cerr << "io_uring_submit failed\n";
      error_code = ret;
      handle.resume();
    }
  }

  ReadvAwaitable(int fd, struct iovec *iovs, size_t num, io_uring *ring)
      : IOAwaitable(ring) {
    req_data.req_type = ReqType::READV;

    auto &readv_data = req_data.specific_data.readv_data;
    readv_data = {fd, iovs, num};
  }
};

struct WritevAwaitable : IOAwaitable<ResponseType::WRITEV, WritevAwaitable> {
  void suspend_action(EvTask::Handle handle) {
    channel = &handle.promise().state.com_data;
    req_data.handle = handle; // just got the handle, so set it

    auto &writev_data = req_data.specific_data.writev_data;
    io_uring_prep_writev(sqe, writev_data.fd, writev_data.iovs, writev_data.num,
                         0);
    io_uring_sqe_set_data(sqe, &req_data);

    auto ret = io_uring_submit(ring);
    if (ret < 0) {
      std::cerr << "io_uring_submit failed\n";
      error_code = ret;
      handle.resume();
    }
  }

  WritevAwaitable(int fd, struct iovec *iovs, size_t num, io_uring *ring)
      : IOAwaitable(ring) {
    req_data.req_type = ReqType::WRITEV;

    auto &writev_data = req_data.specific_data.writev_data;
    writev_data = {fd, iovs, num};
  }
};

struct AcceptAwaitable : IOAwaitable<ResponseType::ACCEPT, AcceptAwaitable> {
  void suspend_action(EvTask::Handle handle) {
    channel = &handle.promise().state.com_data;
    req_data.handle = handle; // just got the handle, so set it

    auto &accept_data = req_data.specific_data.accept_data;
    io_uring_prep_accept(sqe, accept_data.sockfd, accept_data.addr,
                         accept_data.addrlen, 0);
    io_uring_sqe_set_data(sqe, &req_data);

    auto ret = io_uring_submit(ring);
    if (ret < 0) {
      std::cerr << "io_uring_submit failed\n";
      error_code = ret;
      handle.resume();
    }
  }

  AcceptAwaitable(int sockfd, sockaddr *addr, socklen_t *addrlen,
                  io_uring *ring)
      : IOAwaitable(ring) {
    req_data.req_type = ReqType::ACCEPT;

    auto &accept_data = req_data.specific_data.accept_data;
    accept_data = {sockfd, addr, addrlen};
  }
};

struct ConnectAwaitable : IOAwaitable<ResponseType::CONNECT, ConnectAwaitable> {
  void suspend_action(EvTask::Handle handle) {
    channel = &handle.promise().state.com_data;
    req_data.handle = handle; // just got the handle, so set it

    auto &connect_data = req_data.specific_data.connect_data;
    io_uring_prep_connect(sqe, connect_data.sockfd, connect_data.addr,connect_data.addrlen);
    io_uring_sqe_set_data(sqe, &req_data);

    auto ret = io_uring_submit(ring);
    if (ret < 0) {
      std::cerr << "io_uring_submit failed\n";
      error_code = ret;
      handle.resume();
    }
  }

  ConnectAwaitable(int sockfd, const sockaddr* addr, socklen_t addrlen,
                   io_uring *ring)
      : IOAwaitable(ring) {
    req_data.req_type = ReqType::CONNECT;

    auto &connect_data = req_data.specific_data.connect_data;
    connect_data = {sockfd, addr, addrlen};
  }
};

#endif