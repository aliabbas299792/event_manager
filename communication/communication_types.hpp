#ifndef RESPONSE_TYPE_
#define RESPONSE_TYPE_

#include <cstddef>
#include <cstdint>
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
template <RequestType T> struct ResponseDataTypes { using Type = std::nullptr_t; };

template <> struct ResponseDataTypes<RequestType::READ> { using Type = ReadResponsePack; };

template <> struct ResponseDataTypes<RequestType::WRITE> { using Type = WriteResponsePack; };

template <> struct ResponseDataTypes<RequestType::CLOSE> { using Type = CloseResponsePack; };

template <> struct ResponseDataTypes<RequestType::SHUTDOWN> { using Type = ShutdownResponsePack; };

template <> struct ResponseDataTypes<RequestType::READV> { using Type = ReadvResponsePack; };

template <> struct ResponseDataTypes<RequestType::WRITEV> { using Type = WritevResponsePack; };

template <> struct ResponseDataTypes<RequestType::ACCEPT> { using Type = AcceptResponsePack; };

template <> struct ResponseDataTypes<RequestType::CONNECT> { using Type = ConnectResponsePack; };

template <> struct ResponseDataTypes<RequestType::OPENAT> { using Type = OpenatResponsePack; };

template <> struct ResponseDataTypes<RequestType::STATX> { using Type = StatxResponsePack; };

template <> struct ResponseDataTypes<RequestType::UNLINKAT> { using Type = UnlinkatResponsePack; };

template <> struct ResponseDataTypes<RequestType::RENAMEAT> { using Type = RenameatResponsePack; };

template <RequestType Rt> using RespDataTypeMap = typename ResponseDataTypes<Rt>::Type;

using ResponseVariant =
    std::variant<RespDataTypeMap<RequestType::READ>, RespDataTypeMap<RequestType::WRITE>,
                 RespDataTypeMap<RequestType::CLOSE>, RespDataTypeMap<RequestType::SHUTDOWN>,
                 RespDataTypeMap<RequestType::READV>, RespDataTypeMap<RequestType::WRITEV>,
                 RespDataTypeMap<RequestType::ACCEPT>, RespDataTypeMap<RequestType::CONNECT>,
                 RespDataTypeMap<RequestType::OPENAT>, RespDataTypeMap<RequestType::STATX>,
                 RespDataTypeMap<RequestType::UNLINKAT>, RespDataTypeMap<RequestType::RENAMEAT>,
                 std::monostate>;

#endif