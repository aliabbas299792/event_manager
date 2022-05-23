#include "../event_manager.hpp"
#include "../header/event_manager_metadata.hpp"

int event_manager::shared_ring_fd = -1; // static variable, so must initialise here or somewhere
std::mutex event_manager::init_mutex{};
int event_manager::ring_instances = 0;

void event_manager::set_callbacks(event_manager_callbacks callbacks) {
  this->callbacks = callbacks;
}

void event_manager::kill() {
  uint64_t data = 1;
  write(pfd_data(kill_pfd).fd, &data, sizeof(uint64_t));
}

uint64_t event_manager::create_event_fd_normally() {
  auto efd = eventfd(0, 0);
  if(efd == -1) {
    std::cerr << "Error in making eventfd: " << errno << "\n";
    return 0;
  }

  auto pfd = pfd_data(fd_types::EVENT, max_current_id++, efd);
  return pfd.make_pfd_number();
}

// flags are the same flags as open(2)
uint64_t event_manager::open_normally_get_pfd(const char *pathname, int flags) {
  auto potential_fd = open(pathname, flags);

  if(potential_fd < 0) {
    std::cerr << "Error in opening file: " << errno << "\n";
    return 0;
  }

  auto pfd = pfd_data(fd_types::LOCAL, max_current_id++, potential_fd);
  return pfd.make_pfd_number();
}

// flags are the same flags as open(2)
uint64_t event_manager::open_normally_get_pfd(const char *pathname, int flags, int mode) {
  auto potential_fd = open(pathname, flags, mode);

  if(potential_fd < 0) {
    return 0;
  }

  auto pfd = pfd_data(fd_types::LOCAL, max_current_id++, potential_fd);
  return pfd.make_pfd_number();
}

int event_manager::unlink_normally(const char *name) {
  return unlink(name);
}

int event_manager::stat_normally(const char *path, struct stat* buf) {
  return stat(path, buf);
}

int event_manager::fstat_normally(uint64_t pfd, struct stat* buf) {
  auto fd = pfd_data(pfd).fd;
  return fstat(fd, buf);
}

int event_manager::submit_all_queued_sqes() {
  return io_uring_submit(&ring);
}

int event_manager::submit_read(uint64_t pfd, uint8_t* buffer, size_t length) {
  if(queue_read(pfd, buffer, length) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::submit_write(uint64_t pfd, uint8_t* buffer, size_t length) {
  if(queue_write(pfd, buffer, length) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::submit_accept(int fd) {
  if(queue_accept(fd) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::submit_shutdown(uint64_t pfd, int how) {
  if(queue_shutdown(pfd, how) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::submit_close(uint64_t pfd) {
  if(queue_close(pfd) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::event_alert_normal(uint64_t pfd) {
  return eventfd_write(pfd_data(pfd).fd, 1);
}

int event_manager::submit_generic_event(uint64_t pfd, uint64_t additional_info) {
  return submit_event_read(pfd, additional_info, events::EVENT);
}

int event_manager::queue_generic_event(uint64_t pfd, uint64_t additional_info) {
  return queue_event_read(pfd, additional_info, events::EVENT);
}

int event_manager::submit_event_read(uint64_t pfd, uint64_t additional_info, events event) {
  if(queue_event_read(pfd, additional_info, event) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}

int event_manager::queue_event_read(uint64_t pfd, uint64_t additional_info, events event) {
  auto fd = pfd_data(pfd).fd;

  io_uring_sqe *sqe = io_uring_get_sqe(&ring); //get a valid SQE (correct index and all)

  if(sqe == nullptr) {
    return -1;
  }

  auto data = new request_data();
  data->buffer = reinterpret_cast<uint8_t*>(new char[sizeof(uint64_t)]);
  data->length = sizeof(uint64_t);
  data->ev = event;
  data->fd = fd;
  data->additional_info = additional_info;

  io_uring_prep_read(sqe, fd, data->buffer, sizeof(uint64_t), 0); //don't read at an offset
  io_uring_sqe_set_data(sqe, data);

  return 0;
}

int event_manager::close_pfd(uint64_t pfd) {
  auto pfd_stuff = pfd_data(pfd);
  if(pfd_stuff.type == fd_types::LOCAL) {
    return close(pfd_stuff.fd);
  }else{
    return submit_close(pfd);
  }
}

int event_manager::queue_read(uint64_t pfd, uint8_t* buffer, size_t length) {
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

int event_manager::queue_write(uint64_t pfd, uint8_t* buffer, size_t length) {
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

int event_manager::queue_accept(int fd) {
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

int event_manager::queue_shutdown(uint64_t pfd, int how) {
  auto fd = pfd_data(pfd).fd;
  
  auto data = new request_data();
  data->ev = events::SHUTDOWN;
  data->pfd = pfd;
  data->additional_info = how;

  auto sqe = io_uring_get_sqe(&ring);

  if(sqe == nullptr) {
    return -1;
  }
  
  io_uring_prep_shutdown(sqe, fd, how);
  io_uring_sqe_set_data(sqe, data);

  return 0;
}

int event_manager::queue_close(uint64_t pfd) {
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

event_manager::event_manager() {
  std::unique_lock<std::mutex> init_lock(init_mutex);

  if(shared_ring_fd == -1 || ring_instances == 0) { // uses a shared asynchronous backend for all threads
    io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    shared_ring_fd = ring.ring_fd;
  } else {
    io_uring_params params{};
    params.wq_fd = shared_ring_fd;
    params.flags = IORING_SETUP_ATTACH_WQ;
    io_uring_queue_init_params(QUEUE_DEPTH, &ring, &params);
  }

  ring_instances++;

  submit_event_read(kill_pfd, 0, events::KILL); // to ensure the system responds to the kill() command
}

void event_manager::start() {
  while(!killed) {
    await_single_message();
  }
}

void event_manager::await_single_message() {
  io_uring_cqe *cqe;
  int ret = io_uring_wait_cqe(&ring, &cqe);

  if (ret < 0) {
    perror("io_uring_wait_cqe");
  }
  if (cqe->res < 0) {
    std::cerr << "\t(io_uring request failure)\n";
  }

  auto req_data = reinterpret_cast<request_data*>(io_uring_cqe_get_data(cqe));
  event_handler(cqe->res, req_data);

  io_uring_cqe_seen(&ring, cqe);
  free(req_data);

  if(killed == true) { // clean up all resources if killed
    io_uring_queue_exit(&ring);
    close(pfd_data(kill_pfd).fd);

    ring_instances--; // this instance of the ring is now dead
  }
}

void event_manager::event_handler(int res, request_data* req_data) {
  auto pfd = pfd_data(req_data->pfd);
  if((uint64_t)pfd.fd < fd_id_map.size() && fd_id_map[pfd.fd] != pfd.id) {
    // the pfd id is compared with the id stored in the fd_id_map, must be same for this request to be valid
    if(callbacks.close_cb != nullptr) {
      callbacks.close_cb(this, req_data->pfd);
    }
  }

  switch (req_data->ev) {
    case events::WRITE: {
      if(callbacks.write_cb != nullptr) {
        callbacks.write_cb(
          this,
          processed_data(
            req_data->buffer,
            req_data->progress,
            res, 
            errno, req_data->length),
          req_data->pfd
        );
      }
      break;
    }
    case events::READ: {
      if(callbacks.read_cb != nullptr) {
        callbacks.read_cb(
          this,
          processed_data(
            req_data->buffer,
            req_data->progress,
            res, 
            errno, req_data->length),
          req_data->pfd
        );
      }
      break;
    }
    case events::ACCEPT: {
      auto user_data = reinterpret_cast<sockaddr_storage*>(req_data->buffer);

      auto id = max_current_id++;
      auto pfd_num = pfd_data(fd_types::NETWORK, id, res).make_pfd_number();

      fd_id_map.resize(res+1);
      fd_id_map[res] = id;

      if(callbacks.accept_cb != nullptr) {
        callbacks.accept_cb(this, req_data->fd, user_data, req_data->additional_info, pfd_num);
      }

      free(user_data); // free the sockaddr_storage
      break;
    }
    case events::SHUTDOWN: {
      if(callbacks.shutdown_cb != nullptr) {
        // additional_data stores the "how" parameter for the shutdown call
        callbacks.shutdown_cb(this, req_data->additional_info, req_data->pfd);
      }

      break;
    }
    case events::CLOSE: {
      if(callbacks.close_cb != nullptr) {
        callbacks.close_cb(this, req_data->pfd);
      }

      break;
    }
    case events::EVENT: {
      if(callbacks.event_cb != nullptr) {
        callbacks.event_cb(this, req_data->additional_info, req_data->fd);
      }
      break;
    }
    case events::KILL: {
      killed = true;
      break;
    }
    }
}