#include <cstdint>
#include <iostream>

#include <liburing.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>

constexpr int QUEUE_DEPTH = 256;

enum class events {
  WRITE, READ, ACCEPT, SHUTDOWN, CLOSE, STATX, OPEN
};

enum __attribute__ ((__packed__)) fd_types { // 2 byte enum
  GENERIC_ERROR = 0,
  LOCAL,
  NETWORK
};

struct request_data {
  int fd{};
  int request_id{};
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

uint64_t return functions:
  - 0 valued pfds specially indicate there was an error
  - everything else is a valid pfd

besides that exception, return code of 0 indicates success
*/

class event_manager {
    io_uring ring;

    uint16_t max_current_id{};
public:
  // flags are the same flags as open(2)
  uint64_t open_normally_get_pfd(const char *pathname, int flags) {
    auto potential_fd = open(pathname, flags);

    if(potential_fd < 0) {
      return 0;
    }

    auto pfd = pfd_data(fd_types::LOCAL, max_current_id++, potential_fd);
    return pfd.make_pfd_number();
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

  int submit_read(int pfd, uint8_t* buffer, size_t length) {
    if(queue_read(pfd, buffer, length) == -1) {
      return -2;
    }
    return submit_all_queued_sqes();
  }

  int submit_write(int pfd, uint8_t* buffer, size_t length) {
    if(queue_write(pfd, buffer, length) == -1) {
      return -2;
    }
    return submit_all_queued_sqes();
  }

  int submit_accept(int pfd, sockaddr_storage* client_address, socklen_t *client_address_length) {
    if(queue_accept(pfd, client_address, client_address_length) == -1) {
      return -2;
    }
    return submit_all_queued_sqes();
  }

  int submit_shutdown(int pfd, int how) {
    if(queue_shutdown(pfd, how) == -1) {
      return -2;
    }
    return submit_all_queued_sqes();
  }

  int submit_close(int fd) {
    if(queue_close(fd) == -1) {
      return -2;
    }
    return submit_all_queued_sqes();
  }

  int queue_read(int pfd, uint8_t* buffer, size_t length) {
    auto fd = pfd_data(pfd).fd;

    auto data = new request_data();
    data->buffer = buffer;
    data->length = length;
    data->ev = events::READ;

    auto sqe = io_uring_get_sqe(&ring);

    if(sqe == nullptr) {
      return -1;
    }

    io_uring_prep_read(sqe, fd, buffer, length, 0);
    io_uring_sqe_set_data(sqe, data);

    return 1;
  }

  int queue_write(int pfd, uint8_t* buffer, size_t length) {
    auto fd = pfd_data(pfd).fd;
    
    auto data = new request_data();
    data->buffer = buffer;
    data->length = length;
    data->ev = events::WRITE;

    auto sqe = io_uring_get_sqe(&ring);

    if(sqe == nullptr) {
      return -1;
    }

    io_uring_prep_write(sqe, fd, buffer, length, 0);
    io_uring_sqe_set_data(sqe, data);

    return 0;
  }

  int queue_accept(int fd, sockaddr_storage* client_address, socklen_t *client_address_length) {
    auto data = new request_data();
    data->ev = events::ACCEPT;

    auto sqe = io_uring_get_sqe(&ring);

    if(sqe == nullptr) {
      return -1;
    }
    
    io_uring_prep_accept(sqe, fd, (sockaddr*)client_address, client_address_length, 0);
    io_uring_sqe_set_data(sqe, data);

    return 1;
  }

  int queue_shutdown(int pfd, int how) {
    auto fd = pfd_data(pfd).fd;
    
    auto data = new request_data();
    data->ev = events::SHUTDOWN;

    auto sqe = io_uring_get_sqe(&ring);

    if(sqe == nullptr) {
      return -1;
    }
    
    io_uring_prep_shutdown(sqe, fd, how);
    io_uring_sqe_set_data(sqe, data);

    return 1;
  }

  int queue_close(int pfd) {
    auto fd = pfd_data(pfd).fd;
    
    auto data = new request_data();
    data->ev = events::CLOSE;

    auto sqe = io_uring_get_sqe(&ring);

    if(sqe == nullptr) {
      return -1;
    }
    
    io_uring_prep_close(sqe, fd);
    io_uring_sqe_set_data(sqe, data);

    return 1;
  }

  event_manager() {
    io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
  }

  void start() {
    while(true) {
      io_uring_cqe *cqe;
      int ret = io_uring_wait_cqe(&ring, &cqe);

      if (ret < 0) {
        perror("io_uring_wait_cqe");
      }
      if (cqe->res < 0) {
        std::cerr << "io_uring request failure";
      }

      auto user_data = io_uring_cqe_get_data(cqe);

      // do stuff

      io_uring_cqe_seen(&ring, cqe);
    }
  }
};

int main() {
  event_manager ev{};
  auto pfd = ev.open_normally_get_pfd("test.txt", O_RDWR);

  struct stat* buf = new struct stat;
  ev.fstat_normally(pfd, buf);

  std::cout << buf->st_size << " is size\n";
}