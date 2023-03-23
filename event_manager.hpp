#ifndef EVENT_MANAGER
#define EVENT_MANAGER

#include <set>
#include <vector>

#include <liburing.h>

#include "header/event_manager_metadata.hpp"

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

struct processed_data_vecs {
  struct iovec *iovs{};
  size_t num_vecs{};
  int op_res_num{};

  processed_data_vecs() {}

  processed_data_vecs(struct iovec *iovs, int op_res_num, size_t num_vecs) {
    this->iovs = iovs;
    this->op_res_num = op_res_num;
    this->num_vecs = num_vecs;
  }
};

// inherit from this for this interface
class server_methods {
protected:
  // server_methods must have access to event_manager for it to work, but it shouldn't be public
  event_manager *ev{};

public:
  virtual void accept_callback(int listener_pfd, sockaddr_storage *user_data, socklen_t size, uint64_t pfd,
                               int op_res_num, uint64_t additional_info) {}
  virtual void read_callback(processed_data read_metadata, uint64_t pfd, uint64_t additional_info) {}
  virtual void write_callback(processed_data write_metadata, uint64_t pfd, uint64_t additional_info) {}
  virtual void readv_callback(processed_data_vecs read_metadata, uint64_t pfd, uint64_t additional_info) {}
  virtual void writev_callback(processed_data_vecs write_metadata, uint64_t pfd, uint64_t additional_info) {}
  virtual void event_callback(int pfd, int op_res_num, uint64_t additional_info) {}
  virtual void close_callback(uint64_t pfd, int op_res_num, uint64_t additional_info) {}

  virtual void killed_callback(){}; // once the event_manager has been killed, this will be called

  void set_event_manager(event_manager *ev) { this->ev = ev; }

  virtual ~server_methods() = default;
};

class event_manager {
public:
  enum living_state { NOT_STARTED, LIVING, DYING_STAGE_1, DYING_STAGE_2_CANCELLING_REQS, DEAD };
  living_state get_living_state() { return manager_life_state; }
  bool is_dying_or_dead() { return manager_life_state >= DYING_STAGE_1; }

private:
  static int shared_ring_fd;
  static int ring_instances;
  static std::mutex init_mutex;

  io_uring ring{};
  living_state manager_life_state = NOT_STARTED;

  int current_num_of_queued_sqes{};

  server_methods default_server_methods{};
  server_methods *callbacks{};

  int kill_pfd = -1; // initialised later

  // manages how the event manager dies
  void dying_stage_1();
  void dying_stage_2();
  void dying_stage_3();
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

  // used for doing read ops which shouldn't reach the user
  int submit_read_internal(int pfd, uint8_t *buffer, size_t length, events e, uint64_t additional_info = -1);
  int queue_read_internal(int pfd, uint8_t *buffer, size_t length, events e, uint64_t additional_info = -1);

  int submit_event_read(int pfd, uint64_t additional_info, events event);
  int queue_event_read(int pfd, uint64_t additional_info, events event);

  int submit_shutdown(int pfd, int how, uint64_t additional_info = -1);
  int queue_shutdown(int pfd, int how, uint64_t additional_info = -1);
  int submit_close(int pfd, uint64_t additional_info = -1);
  int queue_close(int pfd, uint64_t additional_info = -1);
  int shutdown_and_close_normally(int pfd, int additional_info);
  uint8_t post_shutdown_read_byte{}; // used as a 1 byte buffer to read into during shutdown

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

  // returns pfd for use with other event manager methods - assumes this isn't an eventfd
  int pass_fd_to_event_manager(int fd, bool is_network_fd);

  // generic fd submit ops (i.e calls submit_all_queues_sqes() immediately)
  int submit_read(int pfd, uint8_t *buffer, size_t length, uint64_t additional_info = -1);
  int submit_readv(int pfd, struct iovec *iovs, size_t num, uint64_t additional_info = -1);
  int submit_write(int pfd, uint8_t *buffer, size_t length, uint64_t additional_info = -1);
  int submit_writev(int pfd, struct iovec *iovs, size_t num, uint64_t additional_info = -1);
  int submit_accept(int pfd, uint64_t additional_info = -1);
  int submit_all_queued_sqes();

  int close_pfd(int pfd, uint64_t additional_info = -1);

  // generic fd queue ops (just queues data in the ring without submitting
  // anything)
  int queue_read(int pfd, uint8_t *buffer, size_t length, uint64_t additional_info = -1);
  int queue_readv(int pfd, struct iovec *iovs, size_t num, uint64_t additional_info = -1);
  int queue_write(int pfd, uint8_t *buffer, size_t length, uint64_t additional_info = -1);
  int queue_writev(int pfd, struct iovec *iovs, size_t num, uint64_t additional_info = -1);
  int queue_accept(int pfd, uint64_t additional_info = -1);

  int get_num_queued_sqes() const;

  ~event_manager();
};

#endif