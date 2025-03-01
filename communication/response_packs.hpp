#ifndef RESPONSE_PACK_
#define RESPONSE_PACK_

#include <cstddef>
#include <cstdint>

struct GenericResponsePack {
  int error_num{};
  int req_fd{};
};

struct ReadResponsePack : GenericResponsePack {
  size_t bytes_read{};
  uint8_t* buff{};
};

struct WriteResponsePack : GenericResponsePack {
  size_t bytes_wrote{};
};

struct CloseResponsePack : GenericResponsePack {};

struct ShutdownResponsePack : GenericResponsePack {};

struct ReadvResponsePack : GenericResponsePack {
  size_t bytes_read{};
  uint8_t* buff{};
};

struct WritevResponsePack : GenericResponsePack {
  size_t bytes_wrote{};
};

struct AcceptResponsePack : GenericResponsePack {
  int fd{};
};

struct ConnectResponsePack : GenericResponsePack {};

struct OpenatResponsePack : GenericResponsePack {};

struct StatxResponsePack : GenericResponsePack {
  const char* pathname{};
};

struct UnlinkatResponsePack : GenericResponsePack {
  const char* pathname{};
};

struct RenameatResponsePack : GenericResponsePack {
  const char* oldpathname{};
  const char* newpathname{};
};

#endif