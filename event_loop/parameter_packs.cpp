#include "parameter_packs.hpp"

void RequestQueue::queue_read(int fd, uint8_t* buffer, size_t length) {
  req_vec.push_back(ReadParameterPack{fd, buffer, length});
}

void RequestQueue::queue_write(int fd, const uint8_t* buffer, size_t length) {
  req_vec.push_back(WriteParameterPack{fd, buffer, length});
}

void RequestQueue::queue_close(int fd) {
  req_vec.push_back(CloseParameterPack{fd});
}

void RequestQueue::queue_shutdown(int fd, int how) {
  req_vec.push_back(ShutdownParameterPack{fd, how});
}

void RequestQueue::queue_readv(int fd, struct iovec* iovs, size_t num) {
  req_vec.push_back(ReadvParameterPack{fd, iovs, num});
}

void RequestQueue::queue_writev(int fd, struct iovec* iovs, size_t num) {
  req_vec.push_back(WritevParameterPack{fd, iovs, num});
}

void RequestQueue::queue_accept(int sockfd, sockaddr* addr, socklen_t* addrlen) {
  req_vec.push_back(AcceptParameterPack{sockfd, addr, addrlen});
}

void RequestQueue::queue_connect(int sockfd, const sockaddr* addr, socklen_t addrlen) {
  req_vec.push_back(ConnectParameterPack{sockfd, addr, addrlen});
}

void RequestQueue::queue_openat(int dirfd, const char* pathname, int flags, mode_t mode) {
  req_vec.push_back(OpenatParameterPack{dirfd, pathname, flags, mode});
}

void RequestQueue::queue_statx(int dirfd, const char* pathname, int flags, unsigned int mask,
                               struct statx* statxbuf) {
  req_vec.push_back(StatxParameterPack{dirfd, pathname, flags, mask, statxbuf});
}

void RequestQueue::queue_unlinkat(int dirfd, const char* pathname, int flags) {
  req_vec.push_back(UnlinkatParameterPack{dirfd, pathname, flags});
}

void RequestQueue::queue_renameat(int olddirfd, const char* oldpathname, int newdirfd,
                                  const char* newpathname, int flags) {
  req_vec.push_back(RenameatParameterPack{olddirfd, oldpathname, newdirfd, newpathname, flags});
}