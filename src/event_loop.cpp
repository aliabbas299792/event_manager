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
    dying_stage_1();
  }

  if (manager_life_state == living_state::DEAD) {
    dying_stage_3();
  }
}

void event_manager::event_handler(int res, request_data *req_data) {
  if (req_data == nullptr) { // don't have anything to process for requests with no data
    return;
  }

  int pfd = req_data->pfd;
  auto &pfd_info = pfd_to_data[pfd];

  if (manager_life_state == living_state::DYING_STAGE_2_CANCELLING_REQS) {
    dying_stage_2();
  }

  pfd_info.submitted_reqs--; // a request for this pfd is no longer active now

  switch (req_data->ev) {
  case events::WRITE: {
    callbacks->write_callback(processed_data(req_data->buffer, res, req_data->length), pfd,
                              req_data->additional_info);
    break;
  }
  case events::READ_INTERNAL:
  case events::READ: {
    if (req_data->length == 0) {
      // so either from here it submits a shutdown request, or a close request - after this switch we call the close_pfd function
      pfd_info.last_read_zero = true;

      // only close the pfd if the user explicitly tells you to
      // also only call close if the state has changed, not if there was a duplicate call
      if(pfd_info.shutdown_done) {
        close_pfd(pfd, req_data->additional_info);
      }
    }

    if (req_data->ev != events::READ_INTERNAL) {
      callbacks->read_callback(processed_data(req_data->buffer, res, req_data->length), pfd,
                               req_data->additional_info);
    }
    break;
  }
  case events::WRITEV: {
    callbacks->writev_callback(
        processed_data_vecs(reinterpret_cast<struct iovec *>(req_data->buffer), res, req_data->length), pfd,
        req_data->additional_info);
    break;
  }
  case events::READV: {
    callbacks->readv_callback(
        processed_data_vecs(reinterpret_cast<struct iovec *>(req_data->buffer), res, req_data->length), pfd,
        req_data->additional_info);
    break;
  }
  case events::ACCEPT: {
    auto user_data = reinterpret_cast<sockaddr_storage *>(req_data->buffer);
    uint64_t pfd_num{};

    if (res < 0) {
      std::cerr << "accept, event_handler, res < 0: (res is " << res << ")\n";
    } else {
      pfd_num = pfd_make(res);
    }

    callbacks->accept_callback(pfd, user_data, sizeof(*user_data), pfd_num, res, req_data->additional_info);

    delete user_data; // free the sockaddr_storage
    break;
  }
  case events::SHUTDOWN: {
    if (res < 0) {
      // on any sort of error, just try to close immediately
      shutdown_and_close_normally(pfd, req_data->additional_info);
    } else {
      // otherwise we know shutdown has been done - after this switch we call the close_pfd function
      pfd_info.shutdown_done = true;

      // only call close if the state has changed, not if there was a duplicate call
      close_pfd(pfd, req_data->additional_info);
    }

    break;
  }
  case events::CLOSE: {
    pfd_info.is_being_freed = true; // (flag set before so duplicate calls can't be made)
    callbacks->close_callback(pfd, res, req_data->additional_info);

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

  // have to use a new pfd_info reference since previous one may have been invalidated
  auto &pfd_info_after = pfd_to_data[pfd];
  // only free if there are no other in flight requests for this pfd
  if (pfd_info_after.submitted_reqs == 0 && pfd_info_after.is_being_freed) {
    // and only then if the manager isn't dead
    if(manager_life_state != living_state::DEAD) {
      pfd_free(pfd);
    }
  }
}