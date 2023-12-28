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

  ReadParameterPack(int fd, uint8_t *buffer, size_t length)
      : fd(fd), buffer(buffer), length(length) {}
};

struct WriteParameterPack {
  int fd;
  const uint8_t *buffer;
  size_t length;

  WriteParameterPack(int fd, const uint8_t *buffer, size_t length)
      : fd(fd), buffer(buffer), length(length) {}
};

struct CloseParameterPack {
  int fd;

  CloseParameterPack(int fd) : fd(fd) {}
};

struct ShutdownParameterPack {
  int fd;
  int how;

  ShutdownParameterPack(int fd, int how) : fd(fd), how(how) {}
};

struct ReadvParameterPack {
  int fd;
  struct iovec *iovs;
  size_t num;

  ReadvParameterPack(int fd, struct iovec *iovs, size_t num)
      : fd(fd), iovs(iovs), num(num) {}
};

struct WritevParameterPack {
  int fd;
  struct iovec *iovs;
  size_t num;

  WritevParameterPack(int fd, struct iovec *iovs, size_t num)
      : fd(fd), iovs(iovs), num(num) {}
};

struct AcceptParameterPack {
  int sockfd;
  sockaddr *addr;
  socklen_t *addrlen;

  AcceptParameterPack(int sockfd, sockaddr *addr, socklen_t *addrlen)
      : sockfd(sockfd), addr(addr), addrlen(addrlen) {}
};

struct ConnectParameterPack {
  int sockfd;
  const sockaddr *addr;
  socklen_t addrlen;

  ConnectParameterPack(int sockfd, const sockaddr *addr, socklen_t addrlen)
      : sockfd(sockfd), addr(addr), addrlen(addrlen) {}
};

using OperationParameterPackVariant =
    std::variant<ReadParameterPack, WriteParameterPack, CloseParameterPack,
                 ShutdownParameterPack, ReadvParameterPack, WritevParameterPack,
                 AcceptParameterPack, ConnectParameterPack>;

template <RequestType>
struct RequesToParamPack; // Primary template left undefined on purpose.

template <> struct RequesToParamPack<RequestType::READ> {
  using type = ReadParameterPack;
};

template <> struct RequesToParamPack<RequestType::WRITE> {
  using type = WriteParameterPack;
};

template <> struct RequesToParamPack<RequestType::CLOSE> {
  using type = CloseParameterPack;
};

template <> struct RequesToParamPack<RequestType::SHUTDOWN> {
  using type = ShutdownParameterPack;
};

template <> struct RequesToParamPack<RequestType::READV> {
  using type = ReadvParameterPack;
};

template <> struct RequesToParamPack<RequestType::WRITEV> {
  using type = WritevParameterPack;
};

template <> struct RequesToParamPack<RequestType::ACCEPT> {
  using type = AcceptParameterPack;
};

template <> struct RequesToParamPack<RequestType::CONNECT> {
  using type = ConnectParameterPack;
};

using RequestOpVec = std::vector<OperationParameterPackVariant>;

struct RequestQueue {
  RequestOpVec req_vec{};

  void queue_read(int fd, uint8_t *buffer, size_t length) {
    req_vec.push_back(ReadParameterPack(fd, buffer, length));
  }

  void queue_write(int fd, const uint8_t *buffer, size_t length) {
    req_vec.push_back(WriteParameterPack(fd, buffer, length));
  }

  void queue_close(int fd) { req_vec.push_back(CloseParameterPack(fd)); }

  void queue_shutdown(int fd, int how) {
    req_vec.push_back(ShutdownParameterPack(fd, how));
  }

  void queue_readv(int fd, struct iovec *iovs, size_t num) {
    req_vec.push_back(ReadvParameterPack(fd, iovs, num));
  }

  void queue_writev(int fd, struct iovec *iovs, size_t num) {
    req_vec.push_back(WritevParameterPack(fd, iovs, num));
  }

  void queue_accept(int sockfd, sockaddr *addr, socklen_t *addrlen) {
    req_vec.push_back(AcceptParameterPack(sockfd, addr, addrlen));
  }

  void queue_connect(int sockfd, const sockaddr *addr, socklen_t addrlen) {
    req_vec.push_back(ConnectParameterPack(sockfd, addr, addrlen));
  }
};

#endif