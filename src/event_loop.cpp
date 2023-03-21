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
    std::cerr << "\tio_uring request failure with (code, pfd, fd): (" << cqe->res << ", " << req_data->pfd
              << ", " << pfd_to_data[req_data->pfd].fd << ")\n";
  }

  event_handler(cqe->res, req_data);

  io_uring_cqe_seen(&ring, cqe);
  delete req_data;

  if (manager_life_state == living_state::DYING_STAGE_1) { // clean up all resources if killed
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
      manager_life_state = living_state::DYING_STAGE_2_CANCELLING_REQS;
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

  int pfd = req_data->pfd;
  auto &pfd_info = pfd_to_data[pfd];

  if (manager_life_state == living_state::DYING_STAGE_2_CANCELLING_REQS) {
    end_stage_num_to_cancel--;

    if (end_stage_num_to_cancel == 0) {
      manager_life_state = living_state::DEAD;
    }
  }


  pfd_info.submitted_reqs--; // a request for this pfd is no longer active now

  switch (req_data->ev) {
  case events::WRITE: {
    callbacks->write_callback(processed_data(req_data->buffer, res, req_data->length), pfd,
                              req_data->additional_info);
    break;
  }
  case events::READ: {
    callbacks->read_callback(processed_data(req_data->buffer, res, req_data->length), pfd,
                             req_data->additional_info);
    break;
  }
  case events::WRITEV: {
    callbacks->writev_callback(
        processed_data_vecs(reinterpret_cast<struct iovec *>(req_data->buffer), res, req_data->length),
        pfd, req_data->additional_info);
    break;
  }
  case events::READV: {
    callbacks->readv_callback(
        processed_data_vecs(reinterpret_cast<struct iovec *>(req_data->buffer), res, req_data->length),
        pfd, req_data->additional_info);
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

    callbacks->accept_callback(pfd, user_data, sizeof(*user_data), pfd_num, res,
                               req_data->additional_info);

    delete user_data; // free the sockaddr_storage
    break;
  }
  case events::SHUTDOWN: {
    // additional_data stores the "how" parameter for the shutdown call
    callbacks->shutdown_callback(req_data->info, pfd, res, req_data->additional_info);

    break;
  }
  case events::CLOSE: {
    callbacks->close_callback(pfd, res, req_data->additional_info);
    pfd_info.is_being_freed = true;
    
    break;
  }
  case events::EVENT: {
    callbacks->event_callback(pfd, res, req_data->additional_info);

    delete[] req_data->buffer; // allocated in the queue_event_read function
    break;
  }
  case events::KILL: {
    manager_life_state = living_state::DYING_STAGE_1;

    delete[] req_data->buffer; // allocated in the queue_event_read function
    break;
  }
  }

  // only free if there are no other in flight requests for this pfd
  // have to use a new pfd_info reference since previous one may have been invalidated
  auto &pfd_info_after = pfd_to_data[pfd];
  if(pfd_info_after.submitted_reqs == 0 && pfd_info.is_being_freed) {
    pfd_free(pfd);
  }
}