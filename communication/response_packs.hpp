#ifndef RESPONSE_PACK_
#define RESPONSE_PACK_

#include <cstdint>

struct GenericResponsePack {
  int error_num{};
};

struct ReadResponsePack : GenericResponsePack {
  int bytes_read{};
  uint8_t *buff{};
};

struct WriteResponsePack : GenericResponsePack {
  int bytes_wrote{};
};

struct CloseResponsePack : GenericResponsePack {};

struct ShutdownResponsePack : GenericResponsePack {};

struct ReadvResponsePack : GenericResponsePack {
  int bytes_read{};
  uint8_t *buff{};
};

struct WritevResponsePack : GenericResponsePack {
  int bytes_wrote{};
};

struct AcceptResponsePack : GenericResponsePack {
  int fd{};
};

struct ConnectResponsePack : GenericResponsePack {};

#endif