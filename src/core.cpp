// Methods around initialising, starting and stopping the event manager
// As well as some helper methods for setting up

#include <mutex>
#include <iostream>
#include <stdexcept>

#include "event_manager.hpp"

int event_manager::shared_ring_fd = -1; // static variable, so must initialise here or somewhere
std::mutex event_manager::init_mutex{}; // used to ensure there isn't a race condition at startup
int event_manager::ring_instances = 0;

void event_manager::set_server_methods(server_methods *callbacks) { this->callbacks = callbacks; }
int event_manager::get_num_queued_sqes() const { return current_num_of_queued_sqes; }

event_manager::event_manager(server_methods *callbacks) {
  this->callbacks = callbacks;

  kill_pfd = create_event_fd_normally(); // initialised

  std::unique_lock<std::mutex> init_lock(init_mutex);

  if (shared_ring_fd == -1 || ring_instances == 0) { // uses a shared asynchronous backend for all threads
    io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    shared_ring_fd = ring.ring_fd;
  } else {
    io_uring_params params{};
    params.wq_fd = shared_ring_fd;
    params.flags = IORING_SETUP_ATTACH_WQ;
    io_uring_queue_init_params(QUEUE_DEPTH, &ring, &params);
  }

  ring_instances++;

  submit_event_read(kill_pfd, 0,
                    events::KILL); // to ensure the system responds to the kill() command
}

void event_manager::start() {
  if (callbacks == nullptr) {
    throw std::runtime_error("Server methods callbacks must be set");
  }

  while (manager_life_state != living_state::DEAD) {
    await_single_message();
  }
}

void event_manager::kill() {
  uint64_t data = 1;
  write(pfd_to_data[kill_pfd].fd, &data, sizeof(uint64_t));
}

int event_manager::queue_cancel_request_by_pfd(int pfd) {
  auto sqe = io_uring_get_sqe(&ring);

  if (sqe == nullptr) {
    return -1;
  }

  // IORING_ASYNC_CANCEL_FD cancels any request matching that fd,
  // IORING_ASYNC_CANCEL_ALL means to get all that match this criteria
  io_uring_prep_cancel(sqe, nullptr, IORING_ASYNC_CANCEL_FD | IORING_ASYNC_CANCEL_ALL);
  sqe->fd = pfd_to_data[pfd].fd; // use this fd to cancel request

  return 0;
}

int event_manager::submit_cancel_request_by_pfd(int pfd) {
  if (queue_cancel_request_by_pfd(pfd) == -1) {
    return -2;
  }
  return submit_all_queued_sqes();
}