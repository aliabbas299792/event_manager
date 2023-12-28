#ifndef PARAMETER_PACKS_
#define PARAMETER_PACKS_

#include "communication/communication_types.hpp"
#include <cstdint>
#include <sys/socket.h>
#include <sys/types.h>
#include <variant>
#include <vector>

struct ReadParameterPack {
  int fd;
  uint8_t *buffer;
  size_t length;
};

struct WriteParameterPack {
  int fd;
  const uint8_t *buffer;
  size_t length;
};

struct CloseParameterPack {
  int fd;
};

struct ShutdownParameterPack {
  int fd;
  int how;
};

struct ReadvParameterPack {
  int fd;
  struct iovec *iovs;
  size_t num;
};

struct WritevParameterPack {
  int fd;
  struct iovec *iovs;
  size_t num;
};

struct AcceptParameterPack {
  int sockfd;
  sockaddr *addr;
  socklen_t *addrlen;
};

struct ConnectParameterPack {
  int sockfd;
  const sockaddr *addr;
  socklen_t addrlen;
};

using OperationParameterPackVariant =
    std::variant<ReadParameterPack, WriteParameterPack, CloseParameterPack,
                 ShutdownParameterPack, ReadvParameterPack, WritevParameterPack,
                 AcceptParameterPack, ConnectParameterPack>;

template <RequestType>
struct RequestToParamPack;

template <> struct RequestToParamPack<RequestType::READ> {
  using type = ReadParameterPack;
};

template <> struct RequestToParamPack<RequestType::WRITE> {
  using type = WriteParameterPack;
};

template <> struct RequestToParamPack<RequestType::CLOSE> {
  using type = CloseParameterPack;
};

template <> struct RequestToParamPack<RequestType::SHUTDOWN> {
  using type = ShutdownParameterPack;
};

template <> struct RequestToParamPack<RequestType::READV> {
  using type = ReadvParameterPack;
};

template <> struct RequestToParamPack<RequestType::WRITEV> {
  using type = WritevParameterPack;
};

template <> struct RequestToParamPack<RequestType::ACCEPT> {
  using type = AcceptParameterPack;
};

template <> struct RequestToParamPack<RequestType::CONNECT> {
  using type = ConnectParameterPack;
};

using RequestOpVec = std::vector<OperationParameterPackVariant>;

struct RequestQueue {
  RequestOpVec req_vec{};

  void queue_read(int fd, uint8_t *buffer, size_t length) {
    req_vec.push_back(ReadParameterPack{fd, buffer, length});
  }

  void queue_write(int fd, const uint8_t *buffer, size_t length) {
    req_vec.push_back(WriteParameterPack{fd, buffer, length});
  }

  void queue_close(int fd) { req_vec.push_back(CloseParameterPack{fd}); }

  void queue_shutdown(int fd, int how) {
    req_vec.push_back(ShutdownParameterPack{fd, how});
  }

  void queue_readv(int fd, struct iovec *iovs, size_t num) {
    req_vec.push_back(ReadvParameterPack{fd, iovs, num});
  }

  void queue_writev(int fd, struct iovec *iovs, size_t num) {
    req_vec.push_back(WritevParameterPack{fd, iovs, num});
  }

  void queue_accept(int sockfd, sockaddr *addr, socklen_t *addrlen) {
    req_vec.push_back(AcceptParameterPack{sockfd, addr, addrlen});
  }

  void queue_connect(int sockfd, const sockaddr *addr, socklen_t addrlen) {
    req_vec.push_back(ConnectParameterPack{sockfd, addr, addrlen});
  }
};

#endif