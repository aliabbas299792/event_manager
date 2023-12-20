#ifndef REQUEST_TYPE_
#define REQUEST_TYPE_

#include <cstddef>
#include <variant>

// To add a new type, there are 3 modifications needed:
// 1. Add a new enum value for it
// 2. Add a new type trait specialisation for the enum value
// 3. Add RequestTypes<RequestTypes::new_enum_value> to the variant at the
// bottom (before the monostate)
// The variant ensures that IO tasks can deal with all of these request
// types

enum class RequestType : std::size_t {
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
template <RequestType T> struct RequestTypes {
  using Type = std::nullptr_t;
};

template <> struct RequestTypes<RequestType::READ> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::WRITE> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::OPEN> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::CLOSE> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::SHUTDOWN> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::READV> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::WRITEV> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::ACCEPT> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::CONNECT> {
  using Type = float;
};

template <RequestType Rt> using ReqTypeMap = typename RequestTypes<Rt>::Type;

using RequestVariant = std::variant<
    ReqTypeMap<RequestType::READ>, ReqTypeMap<RequestType::WRITE>,
    ReqTypeMap<RequestType::OPEN>, ReqTypeMap<RequestType::CLOSE>,
    ReqTypeMap<RequestType::SHUTDOWN>, ReqTypeMap<RequestType::READV>,
    ReqTypeMap<RequestType::WRITEV>, ReqTypeMap<RequestType::ACCEPT>,
    ReqTypeMap<RequestType::CONNECT>, std::monostate>;

#endif