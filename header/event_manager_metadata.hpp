#ifndef PFD_DATA
#define PFD_DATA

#include <cstddef>
#include <cstdint>

#include "events_enum.hpp"

constexpr int QUEUE_DEPTH = 256;

enum fd_types : uint16_t { // 1 byte enum
  GENERIC_ERROR = 0,
  LOCAL,
  NETWORK,
  EVENT
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

  bool operator==(pfd_data other) { // redefining equality to use only the fd and the id
    return this->fd == other.fd && this->id == other.id;
  }
};

#endif