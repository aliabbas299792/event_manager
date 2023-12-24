#include "event_manager.hpp"

ReadAwaitable EventManager::read(int fd, uint8_t *buffer, size_t length) {
  return ReadAwaitable{fd, buffer, length, &ring};
}

WriteAwaitable EventManager::write(int fd, uint8_t *buffer, size_t length) {
  return WriteAwaitable{fd, buffer, length, &ring};
}

CloseAwaitable EventManager::close(int fd) { return CloseAwaitable{fd, &ring}; }

ShutdownAwaitable EventManager::shutdown(int fd, int how) {
  return ShutdownAwaitable{fd, how, &ring};
}

ReadvAwaitable EventManager::readv(int fd, struct iovec *iovs, size_t num) {
  return ReadvAwaitable{fd, iovs, num, &ring};
}

WritevAwaitable EventManager::writev(int fd, struct iovec *iovs, size_t num) {
  return WritevAwaitable{fd, iovs, num, &ring};
}

AcceptAwaitable EventManager::accept(int sockfd, sockaddr *addr,
                                     socklen_t *addrlen) {
  return AcceptAwaitable{sockfd, addr, addrlen, &ring};
}

ConnectAwaitable EventManager::connect(int sockfd, const sockaddr *addr,
                                       socklen_t addrlen) {
  return ConnectAwaitable{sockfd, addr, addrlen, &ring};
}