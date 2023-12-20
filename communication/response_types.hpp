#ifndef RESPONSE_TYPE_
#define RESPONSE_TYPE_

#include <cstddef>
#include <variant>

// To add a new type, there are 3 modifications needed:
// 1. Add a new enum value for it
// 2. Add a new type trait specialisation for the enum value
// 3. Add ResponseTypes<ResponseTypes::new_enum_value> to the variant at the
// bottom (before the monostate)
// The variant ensures that IO tasks can deal with all of these response
// types

enum ResponseType : std::size_t {
  READ_RESP = 0,
  WRITE_RESP,
  OPEN_RESP,
  CLOSE_RESP,
  SHUTDOWN_RESP,
  READV_RESP,
  WRITEV_RESP,
  ACCEPT_RESP,
  CONNECT_RESP
};

// default unspecialised
template <ResponseType T> struct ResponseTypes {
  using Type = std::nullptr_t;
};

template <> struct ResponseTypes<ResponseType::READ_RESP> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::WRITE_RESP> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::OPEN_RESP> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::CLOSE_RESP> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::SHUTDOWN_RESP> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::READV_RESP> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::WRITEV_RESP> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::ACCEPT_RESP> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::CONNECT_RESP> {
  using Type = float;
};

template <ResponseType Rt> using RespTypeMap = typename ResponseTypes<Rt>::Type;

using ResponseVariant = std::variant<
    RespTypeMap<ResponseType::READ_RESP>, RespTypeMap<ResponseType::WRITE_RESP>,
    RespTypeMap<ResponseType::OPEN_RESP>, RespTypeMap<ResponseType::CLOSE_RESP>,
    RespTypeMap<ResponseType::SHUTDOWN_RESP>,
    RespTypeMap<ResponseType::READV_RESP>, RespTypeMap<ResponseType::WRITEV_RESP>,
    RespTypeMap<ResponseType::ACCEPT_RESP>,
    RespTypeMap<ResponseType::CONNECT_RESP>, std::monostate>;

#endif