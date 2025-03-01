#ifndef RESPONSE_TYPE_
#define RESPONSE_TYPE_

#include <cstddef>
#include <variant>

#include "response_packs.hpp"

enum class RequestType : std::size_t {
  READ = 0,
  WRITE,
  CLOSE,
  SHUTDOWN,
  READV,
  WRITEV,
  ACCEPT,
  CONNECT,
  OPENAT,
  STATX,
  UNLINKAT,
  RENAMEAT
};

// default unspecialised
template <RequestType T>
struct ResponseDataTypes {
  using type = std::nullptr_t;
};

template <>
struct ResponseDataTypes<RequestType::READ> {
  using type = ReadResponsePack;
};

template <>
struct ResponseDataTypes<RequestType::WRITE> {
  using type = WriteResponsePack;
};

template <>
struct ResponseDataTypes<RequestType::CLOSE> {
  using type = CloseResponsePack;
};

template <>
struct ResponseDataTypes<RequestType::SHUTDOWN> {
  using type = ShutdownResponsePack;
};

template <>
struct ResponseDataTypes<RequestType::READV> {
  using type = ReadvResponsePack;
};

template <>
struct ResponseDataTypes<RequestType::WRITEV> {
  using type = WritevResponsePack;
};

template <>
struct ResponseDataTypes<RequestType::ACCEPT> {
  using type = AcceptResponsePack;
};

template <>
struct ResponseDataTypes<RequestType::CONNECT> {
  using type = ConnectResponsePack;
};

template <>
struct ResponseDataTypes<RequestType::OPENAT> {
  using type = OpenatResponsePack;
};

template <>
struct ResponseDataTypes<RequestType::STATX> {
  using type = StatxResponsePack;
};

template <>
struct ResponseDataTypes<RequestType::UNLINKAT> {
  using type = UnlinkatResponsePack;
};

template <>
struct ResponseDataTypes<RequestType::RENAMEAT> {
  using type = RenameatResponsePack;
};

template <RequestType Rt>
using RespDataTypeMap = typename ResponseDataTypes<Rt>::type;

using ResponseVariant =
    std::variant<RespDataTypeMap<RequestType::READ>, RespDataTypeMap<RequestType::WRITE>,
                 RespDataTypeMap<RequestType::CLOSE>, RespDataTypeMap<RequestType::SHUTDOWN>,
                 RespDataTypeMap<RequestType::READV>, RespDataTypeMap<RequestType::WRITEV>,
                 RespDataTypeMap<RequestType::ACCEPT>, RespDataTypeMap<RequestType::CONNECT>,
                 RespDataTypeMap<RequestType::OPENAT>, RespDataTypeMap<RequestType::STATX>,
                 RespDataTypeMap<RequestType::UNLINKAT>, RespDataTypeMap<RequestType::RENAMEAT>,
                 std::monostate>;

#endif
