#include <iostream>

#include <liburing.h>
#include <sys/socket.h>

constexpr int QUEUE_DEPTH = 256;

class event_manager {
    io_uring ring;
public:
  void submit_write(int fd, uint8_t* buffer, size_t length) {

  }

  void submit_read(int fd, uint8_t* buffer, size_t length) {

  }

  void submit_accept(int fd, sockaddr_storage* client_address, socklen_t *client_address_length) {
    
  }

  void submit_shutdown(int fd, int how) {
  }

  void submit_close(int fd) {
    
  }

  void submit_statx(int dirfd, const char *pathname, int flags, uint32_t mask) {
    
  }

  event_manager() {
    io_uring_queue_init(QUEUE_DEPTH, &ring, 0);

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
    }
  }
};

int main() {
  //
}