#ifndef EVENT_MANAGER
#define EVENT_MANAGER

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <set>

#include <fcntl.h>
#include <liburing.h>
#include <mutex>
#include <netinet/in.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#include "header/event_manager_metadata.hpp"
#include "header/events_enum.hpp"

/*
pfd = pseudo fd, they are indexes to 64 bit structs in pfd_to_data and contain:
  - what type of fd they are
  - the id (explained above)
  - the actual fd
So to use most of the functions in event_manager, a pfd is needed
open_get_pfd_normally returns a 32 bit integer which is a pfd

int return functions:
  - return code of -1 indicates a generic error
  - return code -2 indicates error propagated from a submit_* function

- for the submit_* functions, return code of >=1 is expected, since
io_uring_submit returns the number of sqes submitted

return code of 0 indicates success
*/

class event_manager; // forward declaration for the callback struct
struct request_data;

struct processed_data {
  uint8_t *buff{};
  int op_res_num{};
  size_t length{}; // expected length

  processed_data() {}

  processed_data(uint8_t *buff, int op_res_num, size_t length) {
    this->buff = buff;
    this->op_res_num = op_res_num;
    this->length = length;
  }
};

// inherit from this for this interface
class server_methods {
public:
  virtual void accept_callback(int listener_pfd, sockaddr_storage *user_data,
                               socklen_t size, uint64_t pfd, int op_res_num) {}
  virtual void read_callback(processed_data read_metadata, uint64_t pfd) {}
  virtual void write_callback(processed_data write_metadata, uint64_t pfd) {}
  virtual void event_callback(uint64_t additional_info, int pfd, int op_res_num) {}
  virtual void shutdown_callback(int how, uint64_t pfd, int op_res_num) {}
  virtual void close_callback(uint64_t pfd, int op_res_num) {}

  virtual ~server_methods() = default;

  event_manager *ev{}; // server_methods must have access to event_manager for it to work
};

class event_manager {
public:
  enum living_state { LIVING = 0, DYING, DYING_CANCELLING_REQS, DEAD };
  living_state get_living_state() { return manager_life_state; }

private:
  static int shared_ring_fd;
  static int ring_instances;
  static std::mutex init_mutex;

  io_uring ring{};
  living_state manager_life_state = LIVING;

  int current_num_of_queued_sqes{};

  uint16_t max_current_id{};
  std::vector<int> fd_id_map{}; // used to verify if an fd has been reassigned
  server_methods *callbacks{};

  int kill_pfd = -1; // initialised later
private:
  // just for freeing/storing fd related data at a low index
  // so it is assumed, fd values are centered around 0
  std::vector<pfd_data> pfd_to_data{};
  std::set<int> pfd_freed_pfds{}; // sets are ordered so smallest element at start, which we want
  int pfd_make(int fd, fd_types type);
  void pfd_free(int pfd);

  int submit_cancel_request_by_pfd(int pfd);
  int queue_cancel_request_by_pfd(int pfd); // used to cancel in flight requests
  int end_stage_num_to_cancel{};            // only set when manager_life_state ==
                                            // living_state::DEAD, used to cancel all
                                            // requests
  int submit_all_queued_sqes_privately();   // submit_all_queued_sqes but for when
                                            // shutting down

private:
  void await_single_message();
  void event_handler(int res, request_data *req_data);

  int submit_event_read(int pfd, uint64_t additional_info, events event);
  int queue_event_read(int pfd, uint64_t additional_info, events event);

public:
  // the *_normally functions all are just wrappers for
  // the usual way to do those (i.e socket_create_normally is basically just
  // socket(...)), but gives back a pfd (pseudo fd) rather than an actual fd

  // methods for managing the class/class data
  event_manager() : event_manager(nullptr){};
  event_manager(server_methods *callbacks);
  void start();
  void kill();
  const pfd_data &get_pfd_data(int pfd) { return pfd_to_data[pfd]; }
  void set_server_methods(server_methods *callbacks);

  // eventfd methods
  int submit_generic_event(int pfd, uint64_t additional_info);
  int queue_generic_event(int pfd, uint64_t additional_info);
  int create_event_fd_normally();
  int event_alert_normally(int pfd);
  void shutdown_and_close_normally(int pfd); // avoid this unless you really need to use it
  // i.e this is specifically dealing with when the event manager is
  // DYING/DYING_DYING_CANCELLING_REQS/DEAD

  // file ops
  int open_get_pfd_normally(const char *pathname, int flags);           // flags are the same flags as open(2)
  int open_get_pfd_normally(const char *pathname, int flags, int mode); // flags are the same flags as open(2)
  int socket_create_normally(int domain, int type, int protocol);
  int unlink_normally(const char *name);
  int stat_normally(const char *path, struct stat *buf);
  int fstat_normally(int pfd, struct stat *buf);

  // generic fd submit ops (i.e calls submit_all_queues_sqes() immediately)
  int submit_read(int pfd, uint8_t *buffer, size_t length);
  int submit_write(int pfd, uint8_t *buffer, size_t length);
  int submit_accept(int pfd);
  int submit_shutdown(int pfd, int how);
  int submit_close(int pfd);
  int submit_all_queued_sqes();

  int close_pfd(int pfd);

  // generic fd queue ops (just queues data in the ring without submitting
  // anything)
  int queue_read(int pfd, uint8_t *buffer, size_t length);
  int queue_write(int pfd, uint8_t *buffer, size_t length);
  int queue_accept(int pfd);
  int queue_shutdown(int pfd, int how);
  int queue_close(int pfd);

  int get_num_queued_sqes() const;
};

#endif