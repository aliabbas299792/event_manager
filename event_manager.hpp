#ifndef EVENT_MANAGER
#define EVENT_MANAGER

#include <cstddef>
#include <cstdint>
#include <iostream>

#include <liburing.h>
#include <fcntl.h>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <vector>

#include "events_enum.hpp"

/*
pfd = pseudo fd, they are 64 bit structs cast to uint64_t and contain:
  - what type of fd they are
  - the id (explained above)
  - the actual fd
So to use most of the functions in event_manager, a pfd is needed
open_normally_get_pfd returns a 64 bit integer which is a pfd struct cast to uint64_t

int return functions:
  - return code of -1 indicates a generic error
  - return code -2 indicates error propagated from a submit_* function

- for the submit_* functions, return code of >=1 is expected, since io_uring_submit
  returns the number of sqes submitted

uint64_t return functions:
  - 0 valued pfds specially indicate there was an error
  - everything else is a valid pfd

besides that exception, return code of 0 indicates success
*/

class event_manager; // forward declaration for the callback struct
struct request_data;

struct processed_data {
  uint8_t* buff{};
  size_t amount_processed_before{};
  int op_res_now{};
  int error_num{}; // would just contain errno
  size_t length{}; // expected length

  processed_data() {}

  processed_data(uint8_t *buff, size_t amount_processed_before, int op_res_now, int error_num, size_t length) {
    this->buff = buff;
    this->amount_processed_before = amount_processed_before;
    this->op_res_now = op_res_now;
    this->error_num = error_num;
    this->length = length;
  }
};

struct event_manager_callbacks {
  void (*accept_cb)(event_manager *ev, int listener_fd, sockaddr_storage *user_data, socklen_t size, uint64_t pfd);
  void (*read_cb)(event_manager *ev, processed_data read_metadata, uint64_t pfd);
  void (*write_cb)(event_manager *ev, processed_data write_metadata, uint64_t pfd);
  void (*event_cb)(event_manager *ev, uint64_t additional_info, int fd);
  void (*shutdown_cb)(event_manager *ev, int how, uint64_t pfd);
  void (*close_cb)(event_manager *ev, uint64_t pfd);
};

class event_manager {
private:
  static int shared_ring_fd;
  static int ring_instances;
  static std::mutex init_mutex;

  io_uring ring{};
  bool killed{};

  uint16_t max_current_id{};
  std::vector<int> fd_id_map{};
  event_manager_callbacks callbacks{};

  uint64_t kill_pfd = create_event_fd_normally(); // used to kill the start loop
private:
  void await_single_message();
  void event_handler(int res, request_data* req_data);

  int submit_event_read(uint64_t pfd, uint64_t additional_info, events event);
  int queue_event_read(uint64_t pfd, uint64_t additional_info, events event);
public:
  // methods for managing the class/class data
  event_manager();
  void start();
  void kill();
  void set_callbacks(event_manager_callbacks callbacks);

  // eventfd methods
  uint64_t create_event_fd_normally();
  int submit_generic_event(uint64_t pfd, uint64_t additional_info);
  int queue_generic_event(uint64_t pfd, uint64_t additional_info);
  int event_alert_normal(uint64_t pfd);

  // file ops
  uint64_t open_normally_get_pfd(const char *pathname, int flags); // flags are the same flags as open(2)
  uint64_t open_normally_get_pfd(const char *pathname, int flags, int mode); // flags are the same flags as open(2)
  int unlink_normally(const char *name);
  int stat_normally(const char *path, struct stat* buf);
  int fstat_normally(uint64_t pfd, struct stat* buf);

  // generic fd submit ops (i.e calls submit_all_queues_sqes() immediately)
  int submit_read(uint64_t pfd, uint8_t* buffer, size_t length);
  int submit_write(uint64_t pfd, uint8_t* buffer, size_t length);
  int submit_accept(int fd);
  int submit_shutdown(uint64_t pfd, int how);
  int submit_close(uint64_t pfd);
  int close_pfd(uint64_t pfd);
  int submit_all_queued_sqes();
  
  // generic fd queue ops (just queues data in the ring without submitting anything)
  int queue_read(uint64_t pfd, uint8_t* buffer, size_t length);
  int queue_write(uint64_t pfd, uint8_t* buffer, size_t length);
  int queue_accept(int fd);
  int queue_shutdown(uint64_t pfd, int how);
  int queue_close(uint64_t pfd);
};

#endif