#include "coroutine/io_awaitables.hpp"
#include "coroutine/task.hpp"
#include "event_manager.hpp"

ReadAwaitable EventManager::read(int fd, uint8_t *buffer, size_t length) {
  if (should_restrict_usage())
    return {};
  return ReadAwaitable{fd, buffer, length, this};
}

WriteAwaitable EventManager::write(int fd, const uint8_t *buffer,
                                   size_t length) {
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

EvTask EventManager::queue_read(int fd, uint8_t *buffer, size_t length) {
  if (should_restrict_usage())
    co_return -1;
  co_await QueueOperation(ReadParameterPack(fd, buffer, length));
  co_return 0;
}

EvTask EventManager::queue_write(int fd, const uint8_t *buffer, size_t length) {
  if (should_restrict_usage())
    co_return -1;
  std::cout << "queue write\n";
  co_await QueueOperation(WriteParameterPack(fd, buffer, length));
  std::cout << "queue write after\n";
  co_return 0;
}

EvTask EventManager::queue_close(int fd) {
  if (should_restrict_usage())
    co_return -1;
  co_await QueueOperation(CloseParameterPack(fd));
  co_return 0;
}

EvTask EventManager::queue_shutdown(int fd, int how) {
  if (should_restrict_usage())
    co_return -1;
  co_await QueueOperation(ShutdownParameterPack(fd, how));
  co_return 0;
}

EvTask EventManager::queue_readv(int fd, struct iovec *iovs, size_t num) {
  if (should_restrict_usage())
    co_return -1;
  co_await QueueOperation(ReadvParameterPack(fd, iovs, num));
  co_return 0;
}

EvTask EventManager::queue_writev(int fd, struct iovec *iovs, size_t num) {
  if (should_restrict_usage())
    co_return -1;
  co_await QueueOperation(WritevParameterPack(fd, iovs, num));
  co_return 0;
}

EvTask EventManager::queue_accept(int sockfd, sockaddr *addr,
                                  socklen_t *addrlen) {
  if (should_restrict_usage())
    co_return -1;
  co_await QueueOperation(AcceptParameterPack(sockfd, addr, addrlen));
  co_return 0;
}

EvTask EventManager::queue_connect(int sockfd, const sockaddr *addr,
                                   socklen_t addrlen) {
  if (should_restrict_usage())
    co_return -1;
  co_await QueueOperation(ConnectParameterPack(sockfd, addr, addrlen));
  co_return 0;
}

EvTask EventManager::dispatch_requests(
    ObtainQueueOperations::RequestVec requests_vec) {
  std::cout << "disaptchin\n";
  for (auto &req : requests_vec) {
    auto req_type = static_cast<RequestType>(req.index());
    switch (req_type) {
    case RequestType::READ: {
      auto *pack = std::get_if<ReadParameterPack>(&req);
      if (pack) {
        auto ret = co_await ReadAwaitable(pack->fd, pack->buffer, pack->length,
                                          this, false);
        if (ret.event_system_error < EventSystemError::NO_ERROR) {
          std::cerr << "There was an error in making a request (event manager "
                       "system error for queueing)\n";
          co_return static_cast<int>(ret.event_system_error);
        }
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::WRITE: {
      auto *pack = std::get_if<WriteParameterPack>(&req);
      if (pack) {
        auto ret = co_await WriteAwaitable(pack->fd, pack->buffer, pack->length,
                                           this, false);
        if (ret.event_system_error < EventSystemError::NO_ERROR) {
          std::cerr << "There was an error in making a request (event manager "
                       "system error for queueing)\n";
          co_return static_cast<int>(ret.event_system_error);
        }
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::CLOSE: {
      auto *pack = std::get_if<CloseParameterPack>(&req);
      if (pack) {
        auto ret = co_await CloseAwaitable(pack->fd, this, false);
        if (ret.event_system_error < EventSystemError::NO_ERROR) {
          std::cerr << "There was an error in making a request (event manager "
                       "system error for queueing)\n";
          co_return static_cast<int>(ret.event_system_error);
        }
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::SHUTDOWN: {
      auto *pack = std::get_if<ShutdownParameterPack>(&req);
      if (pack) {
        auto ret = co_await ShutdownAwaitable(pack->fd, pack->how, this, false);
        if (ret.event_system_error < EventSystemError::NO_ERROR) {
          std::cerr << "There was an error in making a request (event manager "
                       "system error for queueing)\n";
          co_return static_cast<int>(ret.event_system_error);
        }
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::READV: {
      auto *pack = std::get_if<ReadvParameterPack>(&req);
      if (pack) {
        auto ret = co_await ReadvAwaitable(pack->fd, pack->iovs, pack->num,
                                           this, false);
        if (ret.event_system_error < EventSystemError::NO_ERROR) {
          std::cerr << "There was an error in making a request (event manager "
                       "system error for queueing)\n";
          co_return static_cast<int>(ret.event_system_error);
        }
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::WRITEV: {
      auto *pack = std::get_if<WritevParameterPack>(&req);
      if (pack) {
        auto ret = co_await WritevAwaitable(pack->fd, pack->iovs, pack->num,
                                            this, false);
        if (ret.event_system_error < EventSystemError::NO_ERROR) {
          std::cerr << "There was an error in making a request (event manager "
                       "system error for queueing)\n";
          co_return static_cast<int>(ret.event_system_error);
        }
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::ACCEPT: {
      auto *pack = std::get_if<AcceptParameterPack>(&req);
      if (pack) {
        auto ret = co_await AcceptAwaitable(pack->sockfd, pack->addr,
                                            pack->addrlen, this, false);
        if (ret.event_system_error < EventSystemError::NO_ERROR) {
          std::cerr << "There was an error in making a request (event manager "
                       "system error for queueing)\n";
          co_return static_cast<int>(ret.event_system_error);
        }
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::CONNECT: {
      auto *pack = std::get_if<ConnectParameterPack>(&req);
      if (pack) {
        auto ret = co_await ConnectAwaitable(pack->sockfd, pack->addr,
                                             pack->addrlen, this, false);
        if (ret.event_system_error < EventSystemError::NO_ERROR) {
          std::cerr << "There was an error in making a request (event manager "
                       "system error for queueing)\n";
          co_return static_cast<int>(ret.event_system_error);
        }
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    }
  }

  co_return 0;
}