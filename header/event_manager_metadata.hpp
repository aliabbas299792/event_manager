#ifndef PFD_DATA
#define PFD_DATA

#include <cstddef>
#include <cstdint>

#include "events_enum.hpp"

// how many items can be in the submission queue at once
constexpr int QUEUE_DEPTH = 256;

enum fd_types : uint16_t { // 2 byte enum
  GENERIC_ERROR = 0,
  LOCAL,
  NETWORK,
  EVENT
};

struct request_data {
  uint64_t pfd{};
  events ev{};

  uint8_t *buffer{};
  size_t length{};
  uint64_t info{};
  uint64_t additional_info{};
};

struct pfd_data {
  fd_types type{};
  int fd = -1;
  // how many in flight requests there are
  int submitted_reqs{};
  // is this pfd in the process of being freed? - true after being closed
  bool is_being_freed{};

  // the flags below are used to manage the graceful shutdown/close procedure (check close_pfd(...) for more)
  // have we submitted a request to check for a read of size zero?
  bool read_zero_check_initiated{};
  // if the last read is zero we can shutdown and close
  bool last_read_zero{};
  // if it is already shutdown, can close
  bool shutdown_done{};
};

#endif