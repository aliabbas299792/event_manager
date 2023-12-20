#include "request_types.hpp"
#include "response_types.hpp"
#include <optional>
#include <variant>

struct CommunicationChannel {
  RequestVariant request_store_{std::monostate{}};
  ResponseVariant response_store_{std::monostate{}};

  template <ResponseType Rt, typename RespType = RespTypeMap<Rt>>
  CommunicationChannel &set_resp_data(RespType &&data) {
    response_store_.emplace<Rt>(std::forward<RespType>(data));
    return *this;
  }

  template <ResponseType Rt, typename RespType = RespTypeMap<Rt>>
  std::optional<RespType> get_resp_data() {
    if (auto data = std::get_if<Rt>(&response_store_)) {
      response_store_.emplace<std::monostate>();
      return std::optional<RespType>(*data);
    } else {
      return std::nullopt;
    }
  }

  template <RequestType Rt, typename ReqType = ReqTypeMap<Rt>>
  CommunicationChannel &set_req_data(ReqType &&data) {
    request_store_.emplace<Rt>(std::forward<ReqType>(data));
    return *this;
  }

  template <RequestType Rt, typename ReqType = ReqTypeMap<Rt>>
  std::optional<ReqType> get_req_data() {
    if (auto data = std::get_if<Rt>(&request_store_)) {
      request_store_.emplace<std::monostate>();
      return std::optional<ReqType>(*data);
    } else {
      return std::nullopt;
    }
  }
};
