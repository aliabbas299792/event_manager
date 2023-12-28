#ifndef PARAMETER_PACKS_
#define PARAMETER_PACKS_

#include "communication/communication_types.hpp"
#include "communication/response_packs.hpp"
#include <cstdint>
#include <liburing.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <variant>
#include <vector>

struct ReadParameterPack {
  int fd{};
  uint8_t *buffer{};
  size_t length{};
};

struct WriteParameterPack {
  int fd{};
  const uint8_t *buffer{};
  size_t length{};
};

struct CloseParameterPack {
  int fd{};
};

struct ShutdownParameterPack {
  int fd{};
  int how{};
};

struct ReadvParameterPack {
  int fd{};
  struct iovec *iovs{};
  size_t num{};
};

struct WritevParameterPack {
  int fd{};
  struct iovec *iovs{};
  size_t num{};
};

struct AcceptParameterPack {
  int sockfd{};
  sockaddr *addr{};
  socklen_t *addrlen{};
};

struct ConnectParameterPack {
  int sockfd{};
  const sockaddr *addr{};
  socklen_t addrlen{};
};

struct OpenatParameterPack {
  int dirfd{};
  const char *pathname{};
  int flags{};
  mode_t mode{};
};

struct StatxParameterPack {
  int dirfd{};
  const char *pathname{};
  int flags{};
  unsigned int mask{};
  struct statx *statxbuf{};
};

struct UnlinkatParameterPack {
  int dirfd{};
  const char *pathname{};
  int flags{};
};

struct RenameatParameterPack {
  int olddirfd{};
  const char *oldpathname{};
  int newdirfd{};
  const char *newpathname{};
  int flags{};
};

using OperationParameterPackVariant =
    std::variant<ReadParameterPack, WriteParameterPack, CloseParameterPack,
                 ShutdownParameterPack, ReadvParameterPack, WritevParameterPack,
                 AcceptParameterPack, ConnectParameterPack, OpenatParameterPack,
                 StatxParameterPack, UnlinkatParameterPack,
                 RenameatParameterPack>;

template <RequestType> struct RequestToParamPack;

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

template <> struct RequestToParamPack<RequestType::OPENAT> {
  using type = OpenatParameterPack;
};

template <> struct RequestToParamPack<RequestType::STATX> {
  using type = StatxParameterPack;
};

template <> struct RequestToParamPack<RequestType::UNLINKAT> {
  using type = UnlinkatParameterPack;
};

template <> struct RequestToParamPack<RequestType::RENAMEAT> {
  using type = RenameatParameterPack;
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

  void queue_openat(int dirfd, const char *pathname, int flags, mode_t mode) {
    req_vec.push_back(OpenatParameterPack{dirfd, pathname, flags, mode});
  }

  void queue_statx(int dirfd, const char *pathname, int flags,
                   unsigned int mask, struct statx *statxbuf) {
    req_vec.push_back(
        StatxParameterPack{dirfd, pathname, flags, mask, statxbuf});
  }

  void queue_unlinkat(int dirfd, const char *pathname, int flags) {
    req_vec.push_back(UnlinkatParameterPack{dirfd, pathname, flags});
  }

  void queue_renameat(int olddirfd, const char *oldpathname, int newdirfd,
                      const char *newpathname, int flags) {
    req_vec.push_back(RenameatParameterPack{olddirfd, oldpathname, newdirfd,
                                            newpathname, flags});
  }
};

#endif