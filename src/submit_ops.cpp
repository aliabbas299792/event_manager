#include <iostream>
#include <sys/eventfd.h>
#include <sys/socket.h>

#include "event_manager.hpp"
#include "header/event_manager_metadata.hpp"

int event_manager::event_alert_normally(int pfd) { return eventfd_write(pfd_to_data[pfd].fd, 1); }

int event_manager::submit_all_queued_sqes() {
  if (is_dying_or_dead()) {
    std::cerr << __FUNCTION__ << " ## " << __LINE__ << " (killed)\n";
    return return_codes::EVENT_MANAGER_HAS_ALREADY_BEEN_KILLED;
  }

  int res = io_uring_submit(&ring);

  current_num_of_queued_sqes = 0; // all are submitted

  // returns the number of SQEs submitted on success
  // on failure returns -errno
  return res;
}

int event_manager::submit_all_queued_sqes_privately() {
  int res = io_uring_submit(&ring);

  current_num_of_queued_sqes = 0; // all are submitted

  return res;
}

int event_manager::submit_read_internal(int pfd, uint8_t *buffer, size_t length, events e,
                                        uint64_t additional_info) {
  auto queue_ret = queue_read_internal(pfd, buffer, length, e, additional_info);
  return queue_ret < 0 ? queue_ret : submit_all_queued_sqes();
}

int event_manager::submit_read(int pfd, uint8_t *buffer, size_t length, uint64_t additional_info) {
  return submit_read_internal(pfd, buffer, length, events::READ, additional_info);
}

int event_manager::submit_write(int pfd, uint8_t *buffer, size_t length, uint64_t additional_info) {
  auto queue_ret = queue_write(pfd, buffer, length, additional_info);
  return queue_ret < 0 ? queue_ret : submit_all_queued_sqes();
}

int event_manager::submit_readv(int pfd, struct iovec *iovs, size_t num, uint64_t additional_info) {
  auto queue_ret = queue_readv(pfd, iovs, num, additional_info);
  return queue_ret < 0 ? queue_ret : submit_all_queued_sqes();
}

int event_manager::submit_writev(int pfd, struct iovec *iovs, size_t num, uint64_t additional_info) {
  auto queue_ret = queue_writev(pfd, iovs, num, additional_info);
  return queue_ret < 0 ? queue_ret : submit_all_queued_sqes();
}

int event_manager::submit_accept(int pfd, uint64_t additional_info) {
  auto queue_ret = queue_accept(pfd, additional_info);
  return queue_ret < 0 ? queue_ret : submit_all_queued_sqes();
}

int event_manager::submit_shutdown(int pfd, int how, uint64_t additional_info) {
  auto queue_ret = queue_shutdown(pfd, how, additional_info);
  return queue_ret < 0 ? queue_ret : submit_all_queued_sqes();
}

int event_manager::submit_close(int pfd, uint64_t additional_info) {
  auto queue_ret = queue_close(pfd, additional_info);
  return queue_ret < 0 ? queue_ret : submit_all_queued_sqes();
}

int event_manager::submit_generic_event(int pfd, uint64_t additional_info) {
  return submit_event_read(pfd, additional_info, events::EVENT);
}

int event_manager::submit_event_read(int pfd, uint64_t additional_info, events event) {
  auto queue_ret = queue_event_read(pfd, additional_info, event);
  return queue_ret < 0 ? queue_ret : submit_all_queued_sqes();
}

int event_manager::shutdown_and_close_normally(int pfd, int additional_info) {
  auto &pfd_info = pfd_to_data[pfd];

  // after this function we either want to close the socket, or have closed it, so these two flags should be true
  pfd_info.last_read_zero = true;
  pfd_info.shutdown_done = true;

  if (pfd_info.submitted_reqs == 0) {
    // if no submitted reqs otherwise shutdown and shutdown and close immediately
    auto fd = pfd_info.fd;
    shutdown(fd, SHUT_RDWR);
    auto ret_close = close(fd);

    // closing is complete - we will free it after this (flag set before so duplicate calls can't be made)
    pfd_info.is_being_freed = true;
    callbacks->close_callback(pfd, ret_close, additional_info);
    pfd_free(pfd);
    return ret_close;
  } else {
    // otherwise mark it to be closed later (flag set before so duplicate calls can't be made)
    pfd_info.is_being_freed = true;
    callbacks->close_callback(pfd, 0, additional_info);
    return return_codes::SUCCESSFUL;
  }
}

// Gracefully closing sockets works like this:
// shutdown -> submit read -> read zero bytes -> close
// or
// read zero bytes -> shutdown -> close

int event_manager::close_pfd(int pfd, uint64_t additional_info) {
  auto &pfd_info = pfd_to_data[pfd];

  if(pfd_info.is_being_freed) {
    // prevent closing if already been closed
    return return_codes::TRYING_TO_CLOSE_PFD_MULTIPLE_TIMES;
  }

  if (pfd_info.last_read_zero && pfd_info.shutdown_done) {
    // shutdown done, last read zero done, so just close
    auto ret_close = submit_close(pfd, additional_info);
    if (ret_close >= 0) {
      return ret_close;
    }

  } else if (pfd_info.shutdown_done) {
    // try to read 1 byte, should cause a zero sized read
    if(!pfd_info.read_zero_check_initiated) {
      auto ret_read = submit_read_internal(pfd, &post_shutdown_read_byte, sizeof(post_shutdown_read_byte),
                                          events::READ_INTERNAL, additional_info);
      if (ret_read >= 0) {
        pfd_info.read_zero_check_initiated = true;
        return ret_read;
      }
    }

  } else {
    // attempt to shutdown network fd, whether or not there has been a zero sized read
    auto ret_shutdown = submit_shutdown(pfd, SHUT_RDWR, additional_info);
    if (ret_shutdown >= 0) {
      return ret_shutdown;
    }
  }

  // otherwise try closing it like a normal pfd
  return shutdown_and_close_normally(pfd, additional_info);
}