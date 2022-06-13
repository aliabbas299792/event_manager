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
  fd_types type{};
  uint16_t id{}; // used to distinguish stale fds from new ones
  // i.e, submit read request, then fd is closed, then new fd
  // with same fd num is opened, then when old fd request
  // returns, then we can distinguish
  // the ID number could be the same as well, but unlikely
  // if you just set it to previous max value + 1
  int fd;
};

#endif