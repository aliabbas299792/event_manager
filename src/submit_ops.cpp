#include <iostream>
#include <sys/eventfd.h>

#include "event_manager.hpp"

int event_manager::event_alert_normally(int pfd) { return eventfd_write(pfd_to_data[pfd].fd, 1); }

int event_manager::submit_all_queued_sqes() {
  if (manager_life_state == living_state::DYING || manager_life_state == living_state::DEAD) {
    std::cerr << __FUNCTION__ << " ## " << __LINE__ << " (killed)\n";
    return -1;
  }

  int res = io_uring_submit(&ring);

  current_num_of_queued_sqes = 0; // all are submitted

  return res;
}

int event_manager::submit_all_queued_sqes_privately() {
  int res = io_uring_submit(&ring);

  current_num_of_queued_sqes = 0; // all are submitted

  return res;
}

int event_manager::submit_read(int pfd, uint8_t *buffer, size_t length, uint64_t additional_info) {
  if (queue_read(pfd, buffer, length, additional_info) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::submit_write(int pfd, uint8_t *buffer, size_t length, uint64_t additional_info) {
  if (queue_write(pfd, buffer, length, additional_info) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::submit_readv(int pfd, struct iovec *iovs, size_t num, uint64_t additional_info) {
  if (queue_readv(pfd, iovs, num, additional_info) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::submit_writev(int pfd, struct iovec *iovs, size_t num, uint64_t additional_info) {
  if (queue_writev(pfd, iovs, num, additional_info) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::submit_accept(int pfd, uint64_t additional_info) {
  if (queue_accept(pfd, additional_info) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

void event_manager::shutdown_and_close_normally(int pfd) {
  auto fd = pfd_to_data[pfd].fd;

  shutdown(fd, SHUT_RDWR);
  close(fd);

  pfd_free(pfd);
}

int event_manager::submit_shutdown(int pfd, int how, uint64_t additional_info) {
  if (queue_shutdown(pfd, how, additional_info) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::submit_close(int pfd, uint64_t additional_info) {
  if (queue_close(pfd, additional_info) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::submit_generic_event(int pfd, uint64_t additional_info) {
  return submit_event_read(pfd, additional_info, events::EVENT);
}

int event_manager::submit_event_read(int pfd, uint64_t additional_info, events event) {
  if (queue_event_read(pfd, additional_info, event) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}