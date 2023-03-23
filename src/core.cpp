// Methods around initialising, starting and stopping the event manager
// As well as some helper methods for setting up

#include <iostream>
#include <mutex>
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

  submit_event_read(kill_pfd, 0,
                    events::KILL); // to ensure the system responds to the kill() command

  // in case none is provided
  callbacks = &default_server_methods;

  ring_instances++;
}

void event_manager::start() {
  if (callbacks == nullptr) {
    throw std::runtime_error("Server methods callbacks must be set (is nullptr)");
  }

  manager_life_state = living_state::LIVING;

  while (manager_life_state != living_state::DEAD) {
    await_single_message();
  }
}

void event_manager::kill() {
  uint64_t data = 1;
  write(pfd_to_data[kill_pfd].fd, &data, sizeof(uint64_t));
}


void event_manager::dying_stage_1() {
  // submit anything in the queue first
  // not using helper function since in DYING state

  for (size_t i = 0; i < pfd_to_data.size(); i++) {
    if (!pfd_freed_pfds.contains(i)) {
      queue_cancel_request_by_pfd(i);

      end_stage_num_to_cancel += pfd_to_data[i].submitted_reqs;
    }

    // don't queue more than can be fit in the queue
    if (current_num_of_queued_sqes >= QUEUE_DEPTH) {
      submit_all_queued_sqes_privately();
    }
  }

  // submit any which haven't been submitted yet
  submit_all_queued_sqes_privately();

  // not dead but not just dying
  if (end_stage_num_to_cancel != 0) {
    manager_life_state = living_state::DYING_STAGE_2_CANCELLING_REQS;
  } else {
    manager_life_state = living_state::DEAD; // nothing left to cancel, we're done
  }
}

void event_manager::dying_stage_2() {
  end_stage_num_to_cancel--;

  if (end_stage_num_to_cancel == 0) {
    manager_life_state = living_state::DEAD;
  }
}

void event_manager::dying_stage_3() {
  // if it is dead, then this is the final iteration of the event loop,
  // so kill the ring
  io_uring_queue_exit(&ring);
  close(pfd_to_data[kill_pfd].fd);

  ring_instances--; // this instance of the ring is now dead

  // this is the only callback that could be tried to be called even if they weren't set
  callbacks->killed_callback(); // event_manager has been killed, call the last callback
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

event_manager::~event_manager() {
  // if the manager wasn't started, there are potentially
  // resources that need to be cleaned, so send a kill signal
  // and start the server
  if (manager_life_state == living_state::NOT_STARTED) {
    if (callbacks == nullptr) {
      // if a nullptr was provided for server methods
      // then just use the default server methods for cleanup
      callbacks = &default_server_methods;
    }

    kill();

    start();
  }
}