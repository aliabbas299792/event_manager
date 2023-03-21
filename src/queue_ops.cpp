#include <iostream>

#include "event_manager.hpp"

int event_manager::queue_event_read(int pfd, uint64_t additional_info, events event) {
  if (is_dying_or_dead()) {
    std::cerr << __FUNCTION__ << " ## " << __LINE__ << " (killed)\n";
    return -1;
  }

  auto &pfd_info = pfd_to_data[pfd];
  auto fd = pfd_info.fd;

  pfd_info.submitted_reqs++;

  io_uring_sqe *sqe = io_uring_get_sqe(&ring); // get a valid SQE (correct index and all)

  if (sqe == nullptr) {
    return -1;
  }

  auto data = new request_data();
  data->buffer = reinterpret_cast<uint8_t *>(new char[sizeof(uint64_t)]);
  data->length = sizeof(uint64_t);
  data->ev = event;
  data->pfd = pfd;
  data->additional_info = additional_info;

  io_uring_prep_read(sqe, fd, data->buffer, sizeof(uint64_t),
                     0); // don't read at an offset
  io_uring_sqe_set_data(sqe, data);

  current_num_of_queued_sqes++;

  return 0;
}

int event_manager::queue_read(int pfd, uint8_t *buffer, size_t length, uint64_t additional_info) {
  if (is_dying_or_dead()) {
    std::cerr << __FUNCTION__ << " ## " << __LINE__ << " (killed)\n";
    return -1;
  }

  auto &pfd_info = pfd_to_data[pfd];
  auto fd = pfd_info.fd;

  pfd_info.submitted_reqs++;

  auto data = new request_data();
  data->buffer = buffer;
  data->length = length;
  data->ev = events::READ;
  data->pfd = pfd;
  data->additional_info = additional_info;

  auto sqe = io_uring_get_sqe(&ring);

  if (sqe == nullptr) {
    return -1;
  }

  io_uring_prep_read(sqe, fd, buffer, length, 0);
  io_uring_sqe_set_data(sqe, data);

  current_num_of_queued_sqes++;

  return 0;
}

int event_manager::queue_write(int pfd, uint8_t *buffer, size_t length, uint64_t additional_info) {
  if (is_dying_or_dead()) {
    std::cerr << __FUNCTION__ << " ## " << __LINE__ << " (killed)\n";
    return -1;
  }

  auto &pfd_info = pfd_to_data[pfd];
  auto fd = pfd_info.fd;

  pfd_info.submitted_reqs++;

  auto data = new request_data();
  data->buffer = buffer;
  data->length = length;
  data->ev = events::WRITE;
  data->pfd = pfd;
  data->additional_info = additional_info;

  auto sqe = io_uring_get_sqe(&ring);

  if (sqe == nullptr) {
    return -1;
  }

  io_uring_prep_write(sqe, fd, buffer, length, 0);
  io_uring_sqe_set_data(sqe, data);

  current_num_of_queued_sqes++;

  return 0;
}

int event_manager::queue_readv(int pfd, struct iovec *iovs, size_t num, uint64_t additional_info) {
  if (is_dying_or_dead()) {
    std::cerr << __FUNCTION__ << " ## " << __LINE__ << " (killed)\n";
    return -1;
  }

  auto &pfd_info = pfd_to_data[pfd];
  auto fd = pfd_info.fd;

  pfd_info.submitted_reqs++;

  auto data = new request_data();
  data->buffer = reinterpret_cast<uint8_t *>(iovs);
  data->length = num;
  data->ev = events::READV;
  data->pfd = pfd;
  data->additional_info = additional_info;

  auto sqe = io_uring_get_sqe(&ring);

  if (sqe == nullptr) {
    return -1;
  }

  io_uring_prep_readv(sqe, fd, iovs, num, 0);
  io_uring_sqe_set_data(sqe, data);

  current_num_of_queued_sqes++;

  return 0;
}

int event_manager::queue_writev(int pfd, struct iovec *iovs, size_t num, uint64_t additional_info) {
  if (is_dying_or_dead()) {
    std::cerr << __FUNCTION__ << " ## " << __LINE__ << " (killed)\n";
    return -1;
  }

  auto &pfd_info = pfd_to_data[pfd];
  auto fd = pfd_info.fd;

  pfd_info.submitted_reqs++;

  auto data = new request_data();
  data->buffer = reinterpret_cast<uint8_t *>(iovs);
  data->length = num;
  data->ev = events::WRITEV;
  data->pfd = pfd;
  data->additional_info = additional_info;

  auto sqe = io_uring_get_sqe(&ring);

  if (sqe == nullptr) {
    return -1;
  }

  io_uring_prep_writev(sqe, fd, iovs, num, 0);
  io_uring_sqe_set_data(sqe, data);

  current_num_of_queued_sqes++;

  return 0;
}

int event_manager::queue_accept(int pfd, uint64_t additional_info) {
  if (is_dying_or_dead()) {
    std::cerr << __FUNCTION__ << " ## " << __LINE__ << " (killed)\n";
    return -1;
  }

  auto &pfd_info = pfd_to_data[pfd];
  auto fd = pfd_info.fd;

  pfd_info.submitted_reqs++;

  auto data = new request_data();
  data->ev = events::ACCEPT;

  auto client_address = new sockaddr_storage;
  data->pfd = pfd;
  data->buffer = reinterpret_cast<uint8_t *>(client_address);
  data->info = sizeof(*client_address);
  data->additional_info = additional_info;

  auto sqe = io_uring_get_sqe(&ring);

  if (sqe == nullptr) {
    return -1;
  }

  io_uring_prep_accept(sqe, fd, (sockaddr *)client_address, reinterpret_cast<uint32_t *>(&data->info), 0);
  io_uring_sqe_set_data(sqe, data);

  current_num_of_queued_sqes++;

  return 0;
}

int event_manager::queue_shutdown(int pfd, int how, uint64_t additional_info) {
  if (is_dying_or_dead()) {
    std::cerr << __FUNCTION__ << " ## " << __LINE__ << " (killed)\n";
    return -1;
  }

  auto &pfd_info = pfd_to_data[pfd];
  auto fd = pfd_info.fd;

  pfd_info.submitted_reqs++;

  auto data = new request_data();
  data->ev = events::SHUTDOWN;
  data->pfd = pfd;
  data->info = how;
  data->additional_info = additional_info;

  auto sqe = io_uring_get_sqe(&ring);

  if (sqe == nullptr) {
    return -1;
  }

  io_uring_prep_shutdown(sqe, fd, how);
  io_uring_sqe_set_data(sqe, data);

  current_num_of_queued_sqes++;

  return 0;
}

int event_manager::queue_close(int pfd, uint64_t additional_info) {
  if (is_dying_or_dead()) {
    std::cerr << __FUNCTION__ << " ## " << __LINE__ << " (killed)\n";
    return -1;
  }

  auto &pfd_info = pfd_to_data[pfd];
  auto fd = pfd_info.fd;

  pfd_info.submitted_reqs++;

  auto data = new request_data();
  data->ev = events::CLOSE;
  data->pfd = pfd;
  data->additional_info = additional_info;

  auto sqe = io_uring_get_sqe(&ring);

  if (sqe == nullptr) {
    return -1;
  }

  io_uring_prep_close(sqe, fd);
  io_uring_sqe_set_data(sqe, data);

  current_num_of_queued_sqes++;

  return 0;
}

int event_manager::queue_generic_event(int pfd, uint64_t additional_info) {
  return queue_event_read(pfd, additional_info, events::EVENT);
}