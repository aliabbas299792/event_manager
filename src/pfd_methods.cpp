#include <iostream>
#include <sys/eventfd.h>

#include "event_manager.hpp"

int event_manager::pfd_make(int fd, fd_types type) {
  if (is_dying_or_dead()) {
    std::cerr << __FUNCTION__ << " ## " << __LINE__ << " (killed)\n";
    return -1;
  }

  int idx = 0;

  if (pfd_freed_pfds.size() > 0) {
    idx = *pfd_freed_pfds.begin();
    pfd_freed_pfds.erase(idx);

    auto &freed_client = pfd_to_data[idx];
    freed_client.is_being_freed = false;
    freed_client.submitted_reqs = 0;
    freed_client.fd = fd;
    freed_client.type = type;
  } else {
    pfd_to_data.emplace_back(type, fd);
    idx = pfd_to_data.size() - 1;
  }

  return idx;
}

void event_manager::pfd_free(int pfd) { pfd_freed_pfds.insert(pfd); }

int event_manager::pass_fd_to_event_manager(int fd, bool is_network_fd) {
  return pfd_make(fd, is_network_fd ? fd_types::NETWORK : fd_types::LOCAL);
}

int event_manager::create_event_fd_normally() {
  auto efd = eventfd(0, 0);
  if (efd == -1) {
    std::cerr << "Error in making eventfd: " << errno << "\n";
    return 0;
  }

  return pfd_make(efd, fd_types::EVENT);
}