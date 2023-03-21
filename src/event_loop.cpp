#include <iostream>

#include "event_manager.hpp"

void event_manager::await_single_message() {
  io_uring_cqe *cqe;
  int ret = io_uring_wait_cqe(&ring, &cqe);

  if (ret < 0) {
    perror("io_uring_wait_cqe");
  }

  auto req_data = reinterpret_cast<request_data *>(io_uring_cqe_get_data(cqe));
  if (cqe->res < 0) {
    std::cerr << "\tio_uring request failure with (code, pfd, fd, id): (" << cqe->res << ", " << req_data->pfd
              << ", " << pfd_to_data[req_data->pfd].fd << ", " << pfd_to_data[req_data->pfd].id << ")\n";
  }

  event_handler(cqe->res, req_data);

  io_uring_cqe_seen(&ring, cqe);
  delete req_data;

  if (manager_life_state == living_state::DYING) { // clean up all resources if killed
    // submit anything in the queue first, not using helper function since in
    // DYING state

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
      manager_life_state = living_state::DYING_CANCELLING_REQS;
    } else {
      manager_life_state = living_state::DEAD; // nothing left to cancel, we're done
    }
  }

  if (manager_life_state == living_state::DEAD) {
    // if it is dead, then this is the final iteration, kill the ring
    io_uring_queue_exit(&ring);
    close(pfd_to_data[kill_pfd].fd);

    ring_instances--; // this instance of the ring is now dead

    callbacks->killed_callback(); // event_manager has been killed, call the last callback
  }
}

void event_manager::event_handler(int res, request_data *req_data) {
  if (req_data == nullptr) { // don't have anything to process for requests with no data
    return;
  }

  auto &pfd_info = pfd_to_data[req_data->pfd];

  if (manager_life_state == living_state::DYING_CANCELLING_REQS) {
    end_stage_num_to_cancel--;

    if (end_stage_num_to_cancel == 0) {
      manager_life_state = living_state::DEAD;
    }
  }

  if (req_data->id != pfd_info.id) {
    // the pfd id is compared with the id stored in the request data
    // for this request to be valid

    // the close callback needn't be called here, since when this fd was replaced
    // by a newer one, then the close callback must've already been called, and
    // new resources unrelated to this iteration of the fd would be cleaned if it
    // were called now

    // clean up resources
    switch (req_data->ev) {
    case events::ACCEPT:
      delete[] req_data->buffer; // free the sockaddr_storage
      break;
    case events::EVENT:
      delete[] req_data->buffer; // allocated in the queue_event_read function
      break;
    default:
      break;
    }

    return;
  }

  pfd_info.submitted_reqs--; // a request for this pfd is no longer active now

  switch (req_data->ev) {
  case events::WRITE: {
    callbacks->write_callback(processed_data(req_data->buffer, res, req_data->length), req_data->pfd,
                              req_data->additional_info);
    break;
  }
  case events::READ: {
    callbacks->read_callback(processed_data(req_data->buffer, res, req_data->length), req_data->pfd,
                             req_data->additional_info);
    break;
  }
  case events::WRITEV: {
    callbacks->writev_callback(
        processed_data_vecs(reinterpret_cast<struct iovec *>(req_data->buffer), res, req_data->length),
        req_data->pfd, req_data->additional_info);
    break;
  }
  case events::READV: {
    callbacks->readv_callback(
        processed_data_vecs(reinterpret_cast<struct iovec *>(req_data->buffer), res, req_data->length),
        req_data->pfd, req_data->additional_info);
    break;
  }
  case events::ACCEPT: {
    auto user_data = reinterpret_cast<sockaddr_storage *>(req_data->buffer);
    uint64_t pfd_num{};

    if (res < 0) {
      std::cerr << "accept, event_handler, res < 0: (res is " << res << ")\n";
    } else {
      pfd_num = pfd_make(res, fd_types::NETWORK);
    }

    callbacks->accept_callback(req_data->pfd, user_data, sizeof(*user_data), pfd_num, res,
                               req_data->additional_info);

    delete user_data; // free the sockaddr_storage
    break;
  }
  case events::SHUTDOWN: {
    // additional_data stores the "how" parameter for the shutdown call
    callbacks->shutdown_callback(req_data->info, req_data->pfd, res, req_data->additional_info);

    break;
  }
  case events::CLOSE: {
    callbacks->close_callback(req_data->pfd, res, req_data->additional_info);

    pfd_free(req_data->pfd);

    break;
  }
  case events::EVENT: {
    callbacks->event_callback(req_data->pfd, res, req_data->additional_info);

    delete[] req_data->buffer; // allocated in the queue_event_read function
    break;
  }
  case events::KILL: {
    manager_life_state = living_state::DYING;

    delete[] req_data->buffer; // allocated in the queue_event_read function
    break;
  }
  }
}