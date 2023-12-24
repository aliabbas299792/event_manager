#include "coroutine/io_awaitables.hpp"
#include "event_manager.hpp"

ReadAwaitable EventManager::read(int fd, uint8_t *buffer, size_t length) {
  if (should_restrict_usage())
    return {};
  return ReadAwaitable{fd, buffer, length, this};
}

WriteAwaitable EventManager::write(int fd, uint8_t *buffer, size_t length) {
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

ReadvAwaitable EventManager::readv(int fd, struct iovec *iovs, size_t num) {
  if (should_restrict_usage())
    return {};
  return ReadvAwaitable{fd, iovs, num, this};
}

WritevAwaitable EventManager::writev(int fd, struct iovec *iovs, size_t num) {
  if (should_restrict_usage())
    return {};
  return WritevAwaitable{fd, iovs, num, this};
}

AcceptAwaitable EventManager::accept(int sockfd, sockaddr *addr,
                                     socklen_t *addrlen) {
  if (should_restrict_usage())
    return {};
  return AcceptAwaitable{sockfd, addr, addrlen, this};
}

ConnectAwaitable EventManager::connect(int sockfd, const sockaddr *addr,
                                       socklen_t addrlen) {
  if (should_restrict_usage())
    return {};
  return ConnectAwaitable{sockfd, addr, addrlen, this};
}