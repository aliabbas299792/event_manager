#ifndef REQUEST_DATA_
#define REQUEST_DATA_

#include <cstdint>
#include <bits/types/struct_iovec.h>
#include <sys/socket.h>

#include "coroutine/task.hpp"

struct ReadRequestData {
  int fd{};
  uint8_t *buffer{};
  size_t length{};
};

struct WriteRequestData {
  int fd{};
  const uint8_t *buffer{};
  size_t length{};
};

struct CloseRequestData {
  int fd{};
};

struct ShutdownRequestData {
  int fd{};
  int how{};
};

struct ReadvRequestData {
  int fd{};
  iovec *iovs{};
  std::size_t num{};
};

struct WritevRequestData {
  int fd{};
  iovec *iovs{};
  size_t num{};
};

struct AcceptRequestData {
  int sockfd{};
  sockaddr *addr{};
  socklen_t *addrlen{};
};

struct ConnectRequestData {
  int sockfd{};
  const sockaddr *addr{};
  socklen_t addrlen{};
};

struct RequestData {
  EvTask::Handle handle{};
  RequestType req_type{};

  union {
    ReadRequestData read_data;
    WriteRequestData write_data;
    CloseRequestData close_data;
    ShutdownRequestData shutdown_data;
    ReadvRequestData readv_data;
    WritevRequestData writev_data;
    AcceptRequestData accept_data;
    ConnectRequestData connect_data;
  } specific_data{};
};

#endif