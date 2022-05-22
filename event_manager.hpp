#ifndef EVENT_MANAGER
#define EVENT_MANAGER

#include <iostream>

#include <liburing.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>

constexpr int QUEUE_DEPTH = 256;

enum class events {
  WRITE, READ, ACCEPT, SHUTDOWN, CLOSE
};

enum __attribute__ ((__packed__)) fd_types { // 2 byte enum
  GENERIC_ERROR = 0,
  LOCAL,
  NETWORK
};

struct request_data {
  int fd{};
  uint64_t pfd{};
  events ev{};

  uint8_t *buffer{};
  size_t length{};
  size_t progress{};
  uint64_t additional_info{};
};

struct pfd_data {
  // can be converted to/from uint64_t because 8 bytes
  fd_types type{};

  uint16_t id{}; // used to distinguish stale fds from new ones
  // i.e, submit read request, then fd is closed, then new fd
  // with same fd num is opened, then when old fd request
  // returns, then we can distinguish
  // the ID number could be the same as well, but unlikely
  // if you just set it to previous max value + 1
  int fd{};

  pfd_data(uint64_t data) {
    this->fd = ((pfd_data*)&data)->fd;
    this->type = ((pfd_data*)&data)->type;
    this->id = ((pfd_data*)&data)->id;
  }

  pfd_data(uint16_t type, uint16_t id, int fd) {
    this->fd = fd;
    this->type = fd_types(type);
    this->id = id;
  }

  uint64_t make_pfd_number() {
    return *((uint64_t*)this);
  }
};

/*
pfd = pseudo fd, they are 64 bit structs cast to uint64_t and contain:
  - what type of fd they are
  - the id (explained above)
  - the actual fd
So to use most of the functions in event_manager, a pfd is needed
open_normally_get_pfd returns a 64 bit integer which is a pfd struct cast

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

struct event_manager_callbacks {
  void (*accept_cb)(event_manager *ev, int listener_fd, sockaddr_storage *user_data, socklen_t size, uint64_t pfd);
  void (*read_cb)(event_manager *ev, uint8_t* buff, size_t amount_read, size_t to_read, uint64_t pfd);
};

class event_manager {
    io_uring ring;

    uint16_t max_current_id{};
    std::vector<int> fd_id_map{};
    event_manager_callbacks callbacks{};
public:
  void set_callbacks(event_manager_callbacks callbacks) {
    this->callbacks = callbacks;
  }

  // flags are the same flags as open(2)
  uint64_t open_normally_get_pfd(const char *pathname, int flags) {
    auto potential_fd = open(pathname, flags);

    if(potential_fd < 0) {
      return 0;
    }

    auto pfd = pfd_data(fd_types::LOCAL, max_current_id++, potential_fd);
    return pfd.make_pfd_number();
  }

  // flags are the same flags as open(2)
  uint64_t open_normally_get_pfd(const char *pathname, int flags, int mode) {
    auto potential_fd = open(pathname, flags, mode);

    if(potential_fd < 0) {
      return 0;
    }

    auto pfd = pfd_data(fd_types::LOCAL, max_current_id++, potential_fd);
    return pfd.make_pfd_number();
  }

  int unlink_normally(const char *name) {
    return unlink(name);
  }

  int stat_normally(const char *path, struct stat* buf) {
    return stat(path, buf);
  }

  int fstat_normally(uint64_t pfd, struct stat* buf) {
    auto fd = pfd_data(pfd).fd;
    return fstat(fd, buf);
  }

  int submit_all_queued_sqes() {
    return io_uring_submit(&ring);
  }

  int submit_read(uint64_t pfd, uint8_t* buffer, size_t length) {
    if(queue_read(pfd, buffer, length) == -1) {
      return -2;
    }
    return submit_all_queued_sqes();
  }

  int submit_write(uint64_t pfd, uint8_t* buffer, size_t length) {
    if(queue_write(pfd, buffer, length) == -1) {
      return -2;
    }
    return submit_all_queued_sqes();
  }

  int submit_accept(int fd) {
    if(queue_accept(fd) == -1) {
      return -2;
    }
    return submit_all_queued_sqes();
  }

  int submit_shutdown(uint64_t pfd, int how) {
    if(queue_shutdown(pfd, how) == -1) {
      return -2;
    }
    return submit_all_queued_sqes();
  }

  int submit_close(uint64_t pfd) {
    if(queue_close(pfd) == -1) {
      return -2;
    }
    return submit_all_queued_sqes();
  }

  int close_pfd(uint64_t pfd) {
    auto pfd_stuff = pfd_data(pfd);
    if(pfd_stuff.type == fd_types::LOCAL) {
      return close(pfd_stuff.fd);
    }else{
      return submit_close(pfd);
    }
  }

  int queue_read(uint64_t pfd, uint8_t* buffer, size_t length) {
    auto fd = pfd_data(pfd).fd;

    auto data = new request_data();
    data->buffer = buffer;
    data->length = length;
    data->ev = events::READ;
    data->pfd = pfd;

    auto sqe = io_uring_get_sqe(&ring);

    if(sqe == nullptr) {
      return -1;
    }

    io_uring_prep_read(sqe, fd, buffer, length, 0);
    io_uring_sqe_set_data(sqe, data);

    return 0;
  }

  int queue_write(uint64_t pfd, uint8_t* buffer, size_t length) {
    auto fd = pfd_data(pfd).fd;
    
    auto data = new request_data();
    data->buffer = buffer;
    data->length = length;
    data->ev = events::WRITE;
    data->pfd = pfd;

    auto sqe = io_uring_get_sqe(&ring);

    if(sqe == nullptr) {
      return -1;
    }

    io_uring_prep_write(sqe, fd, buffer, length, 0);
    io_uring_sqe_set_data(sqe, data);

    return 0;
  }

  int queue_accept(int fd) {
    auto data = new request_data();
    data->ev = events::ACCEPT;

    auto client_address = new sockaddr_storage;
    data->fd = fd;
    data->buffer = reinterpret_cast<uint8_t*>(client_address);
    data->additional_info = sizeof(*client_address);


    auto sqe = io_uring_get_sqe(&ring);

    if(sqe == nullptr) {
      return -1;
    }
    
    io_uring_prep_accept(
      sqe,
      fd, 
      (sockaddr*)client_address,
      reinterpret_cast<uint32_t*>(&data->additional_info), 
      0
    );
    io_uring_sqe_set_data(sqe, data);

    return 0;
  }

  int queue_shutdown(uint64_t pfd, int how) {
    auto fd = pfd_data(pfd).fd;
    
    auto data = new request_data();
    data->ev = events::SHUTDOWN;
    data->pfd = pfd;

    auto sqe = io_uring_get_sqe(&ring);

    if(sqe == nullptr) {
      return -1;
    }
    
    io_uring_prep_shutdown(sqe, fd, how);
    io_uring_sqe_set_data(sqe, data);

    return 0;
  }

  int queue_close(uint64_t pfd) {
    auto fd = pfd_data(pfd).fd;
    
    auto data = new request_data();
    data->ev = events::CLOSE;
    data->pfd = pfd;

    auto sqe = io_uring_get_sqe(&ring);

    if(sqe == nullptr) {
      return -1;
    }
    
    io_uring_prep_close(sqe, fd);
    io_uring_sqe_set_data(sqe, data);

    return 0;
  }

  event_manager() {
    io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
  }

  void start() {
    while(true) {
      await_single_message();
    }
  }

  void await_single_message() {
    io_uring_cqe *cqe;
    int ret = io_uring_wait_cqe(&ring, &cqe);

    if (ret < 0) {
      perror("io_uring_wait_cqe");
    }
    if (cqe->res < 0) {
      std::cerr << "io_uring request failure";
    }

    auto req_data = reinterpret_cast<request_data*>(io_uring_cqe_get_data(cqe));
    event_handler(cqe->res, req_data);

    io_uring_cqe_seen(&ring, cqe);
    free(req_data);
  }

  void event_handler(int res, request_data* req_data) {
    switch (req_data->ev) {
      case events::WRITE:
        break;
      case events::READ: {
        if(callbacks.read_cb != nullptr) {
          callbacks.read_cb(this, req_data->buffer, req_data->progress, req_data->length, req_data->pfd);
        }
        break;
      }
      case events::ACCEPT: {
        auto user_data = reinterpret_cast<sockaddr_storage*>(req_data->buffer);

        auto pfd_num = pfd_data(fd_types::NETWORK, max_current_id++, res).make_pfd_number();

        if(callbacks.accept_cb != nullptr) {
          callbacks.accept_cb(this, req_data->fd, user_data, req_data->additional_info, pfd_num);
        }

        free(user_data); // free the sockaddr_storage
        break;
      }
      case events::SHUTDOWN:
        break;
      case events::CLOSE:
        break;
    }
  }
};

#endif