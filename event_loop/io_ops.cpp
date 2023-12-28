#include "communication/communication_types.hpp"
#include "coroutine/io_awaitables.hpp"
#include "coroutine/task.hpp"
#include "event_loop/parameter_packs.hpp"
#include "event_loop/request_data.hpp"
#include "event_manager.hpp"
#include <cstddef>
#include <liburing.h>

struct RetrieveCurrentHandle {
  EvTask::Handle handle;
  bool await_ready() noexcept { return false; }
  void await_suspend(EvTask::Handle h) noexcept {
    handle = h;
    handle.resume();
  }
  EvTask::Handle await_resume() noexcept { return handle; }
};

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

RequestQueue EventManager::make_request_queue() { return RequestQueue{}; }

EvTask EventManager::submit_and_wait(const RequestQueue &request_queue,
                                     SubmitAndWaitHandler handler) {
  auto &requests_vec = request_queue.req_vec;

  auto ret = submit_queued_entries();
  if (ret < 0) {
    co_return ret;
  }

  std::vector<RequestData> req_data{requests_vec.size()};
  auto handle = co_await RetrieveCurrentHandle{};

  for (std::size_t i = 0; i < requests_vec.size(); i++) {
    auto &req = requests_vec[i];
    auto req_type = static_cast<RequestType>(req.index());

    auto &single_req = req_data[i];
    auto sqe = get_uring_sqe();

    if (sqe == nullptr) {
      std::cerr << "There was an error in making a request (event manager "
                   "system error for queueing)\n";
      co_return -1;
    }

    single_req.handle = handle;
    single_req.req_type = req_type;
    auto &specific_data = single_req.specific_data;

    switch (req_type) {
    case RequestType::READ: {
      auto *pack = std::get_if<ReadParameterPack>(&req);
      if (pack) {
        specific_data.read_data = {pack->fd, pack->buffer, pack->length};
        io_uring_prep_read(sqe, pack->fd, pack->buffer, pack->length, 0);
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::WRITE: {
      auto *pack = std::get_if<WriteParameterPack>(&req);
      if (pack) {
        specific_data.write_data = {pack->fd, pack->buffer, pack->length};
        io_uring_prep_write(sqe, pack->fd, pack->buffer, pack->length, 0);
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::CLOSE: {
      auto *pack = std::get_if<CloseParameterPack>(&req);
      if (pack) {
        specific_data.close_data = {pack->fd};
        io_uring_prep_close(sqe, pack->fd);
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::SHUTDOWN: {
      auto *pack = std::get_if<ShutdownParameterPack>(&req);
      if (pack) {
        specific_data.shutdown_data = {pack->fd};
        io_uring_prep_shutdown(sqe, pack->fd, pack->how);
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::READV: {
      auto *pack = std::get_if<ReadvParameterPack>(&req);
      if (pack) {
        specific_data.readv_data = {pack->fd, pack->iovs, pack->num};
        io_uring_prep_readv(sqe, pack->fd, pack->iovs, pack->num, 0);
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::WRITEV: {
      auto *pack = std::get_if<WritevParameterPack>(&req);
      if (pack) {
        specific_data.writev_data = {pack->fd, pack->iovs, pack->num};
        io_uring_prep_writev(sqe, pack->fd, pack->iovs, pack->num, 0);
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::ACCEPT: {
      auto *pack = std::get_if<AcceptParameterPack>(&req);
      if (pack) {
        specific_data.accept_data = {pack->sockfd, pack->addr, pack->addrlen};
        io_uring_prep_accept(sqe, pack->sockfd, pack->addr, pack->addrlen, 0);
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    case RequestType::CONNECT: {
      auto *pack = std::get_if<ConnectParameterPack>(&req);
      if (pack) {
        specific_data.connect_data = {pack->sockfd, pack->addr, pack->addrlen};
        io_uring_prep_connect(sqe, pack->sockfd, pack->addr, pack->addrlen);
      } else {
        std::cerr << "There was an error in retrieving queued data\n";
        co_return -1;
      }
    } break;
    }

    io_uring_sqe_set_data(sqe, &req_data[i]);
    std::cout << req_data[i].specific_data.write_data.fd << "\n";
  }

  // how many are currently being processed
  auto num_submitted = ret;

  // how many are still queued for submission
  auto num_queued = ring.sq.sqe_tail - ring.sq.sqe_head;

  while (num_submitted != 0 || num_queued != 0) {
    while (num_queued != 0 && num_submitted == 0) {
      auto ret = submit_queued_entries();
      if (ret < 0) {
        co_return ret;
      }

      num_submitted += ret;
      num_queued -= ret;
    }

    for (int i = 0; i < num_submitted; i++) {
      auto channel = co_await GenericResponse{};
      auto response_type = channel->response_store_current_type();
      handler(response_type, channel);
    }

    num_submitted = 0;
  }

  co_return 0;
}