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
template <RequestType T> struct ResponseDataTypes {
  using Type = std::nullptr_t;
};

template <> struct ResponseDataTypes<RequestType::READ> {
  using Type = ReadResponsePack;
};

template <> struct ResponseDataTypes<RequestType::WRITE> {
  using Type = WriteResponsePack;
};

template <> struct ResponseDataTypes<RequestType::CLOSE> {
  using Type = CloseResponsePack;
};

template <> struct ResponseDataTypes<RequestType::SHUTDOWN> {
  using Type = ShutdownResponsePack;
};

template <> struct ResponseDataTypes<RequestType::READV> {
  using Type = ReadvResponsePack;
};

template <> struct ResponseDataTypes<RequestType::WRITEV> {
  using Type = WritevResponsePack;
};

template <> struct ResponseDataTypes<RequestType::ACCEPT> {
  using Type = AcceptResponsePack;
};

template <> struct ResponseDataTypes<RequestType::CONNECT> {
  using Type = ConnectResponsePack;
};

template <RequestType Rt> using RespDataTypeMap = typename ResponseDataTypes<Rt>::Type;

using ResponseVariant = std::variant<
    RespDataTypeMap<RequestType::READ>, RespDataTypeMap<RequestType::WRITE>,
    RespDataTypeMap<RequestType::CLOSE>, RespDataTypeMap<RequestType::SHUTDOWN>,
    RespDataTypeMap<RequestType::READV>, RespDataTypeMap<RequestType::WRITEV>,
    RespDataTypeMap<RequestType::ACCEPT>, RespDataTypeMap<RequestType::CONNECT>,
    std::monostate>;

#endif