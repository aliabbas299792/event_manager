#ifndef RESPONSE_TYPE_
#define RESPONSE_TYPE_

#include <cstddef>
#include <cstdint>
#include <variant>

// To add a new type, there are 3 modifications needed:
// 1. Add a new enum value for it
// 2. Add a new type trait specialisation for the enum value
// 3. Add ResponseTypes<ResponseTypes::new_enum_value> to the variant at the
// bottom (before the monostate)
// The variant ensures that IO tasks can deal with all of these response
// types

enum class RequestType : std::size_t {
  READ = 0,
  WRITE,
  CLOSE,
  SHUTDOWN,
  READV,
  WRITEV,
  ACCEPT,
  CONNECT
};

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

// default unspecialised
template <RequestType T> struct ResponseTypes {
  using Type = std::nullptr_t;
};

template <> struct ResponseTypes<RequestType::READ> {
  using Type = ReadResponsePack;
};

template <> struct ResponseTypes<RequestType::WRITE> {
  using Type = WriteResponsePack;
};

template <> struct ResponseTypes<RequestType::CLOSE> {
  using Type = CloseResponsePack;
};

template <> struct ResponseTypes<RequestType::SHUTDOWN> {
  using Type = ShutdownResponsePack;
};

template <> struct ResponseTypes<RequestType::READV> {
  using Type = ReadvResponsePack;
};

template <> struct ResponseTypes<RequestType::WRITEV> {
  using Type = WritevResponsePack;
};

template <> struct ResponseTypes<RequestType::ACCEPT> {
  using Type = AcceptResponsePack;
};

template <> struct ResponseTypes<RequestType::CONNECT> {
  using Type = ConnectResponsePack;
};

template <RequestType Rt> using RespTypeMap = typename ResponseTypes<Rt>::Type;

using ResponseVariant = std::variant<
    RespTypeMap<RequestType::READ>, RespTypeMap<RequestType::WRITE>,
    RespTypeMap<RequestType::CLOSE>, RespTypeMap<RequestType::SHUTDOWN>,
    RespTypeMap<RequestType::READV>, RespTypeMap<RequestType::WRITEV>,
    RespTypeMap<RequestType::ACCEPT>, RespTypeMap<RequestType::CONNECT>,
    std::monostate>;

#endif