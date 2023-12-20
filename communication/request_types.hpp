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

enum RequestType : std::size_t {
  READ_REQ = 0,
  WRITE_REQ,
  OPEN_REQ,
  CLOSE_REQ,
  SHUTDOWN_REQ,
  READV_REQ,
  WRITEV_REQ,
  ACCEPT_REQ,
  CONNECT_REQ
};

// default unspecialised
template <RequestType T> struct RequestTypes {
  using Type = std::nullptr_t;
};

template <> struct RequestTypes<RequestType::READ_REQ> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::WRITE_REQ> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::OPEN_REQ> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::CLOSE_REQ> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::SHUTDOWN_REQ> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::READV_REQ> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::WRITEV_REQ> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::ACCEPT_REQ> {
  using Type = float;
};

template <> struct RequestTypes<RequestType::CONNECT_REQ> {
  using Type = float;
};

template <RequestType Rt> using ReqTypeMap = typename RequestTypes<Rt>::Type;

using RequestVariant = std::variant<
    ReqTypeMap<RequestType::READ_REQ>, ReqTypeMap<RequestType::WRITE_REQ>,
    ReqTypeMap<RequestType::OPEN_REQ>, ReqTypeMap<RequestType::CLOSE_REQ>,
    ReqTypeMap<RequestType::SHUTDOWN_REQ>, ReqTypeMap<RequestType::READV_REQ>,
    ReqTypeMap<RequestType::WRITEV_REQ>, ReqTypeMap<RequestType::ACCEPT_REQ>,
    ReqTypeMap<RequestType::CONNECT_REQ>, std::monostate>;

#endif