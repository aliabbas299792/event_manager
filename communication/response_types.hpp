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

enum class ResponseType : std::size_t {
  READ = 0,
  WRITE,
  OPEN,
  CLOSE,
  SHUTDOWN,
  READV,
  WRITEV,
  ACCEPT,
  CONNECT
};

// default unspecialised
template <ResponseType T> struct ResponseTypes {
  using Type = std::nullptr_t;
};

template <> struct ResponseTypes<ResponseType::READ> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::WRITE> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::OPEN> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::CLOSE> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::SHUTDOWN> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::READV> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::WRITEV> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::ACCEPT> {
  using Type = float;
};

template <> struct ResponseTypes<ResponseType::CONNECT> {
  using Type = float;
};

template <ResponseType Rt> using RespTypeMap = typename ResponseTypes<Rt>::Type;

using ResponseVariant = std::variant<
    RespTypeMap<ResponseType::READ>, RespTypeMap<ResponseType::WRITE>,
    RespTypeMap<ResponseType::OPEN>, RespTypeMap<ResponseType::CLOSE>,
    RespTypeMap<ResponseType::SHUTDOWN>,
    RespTypeMap<ResponseType::READV>, RespTypeMap<ResponseType::WRITEV>,
    RespTypeMap<ResponseType::ACCEPT>,
    RespTypeMap<ResponseType::CONNECT>, std::monostate>;

#endif