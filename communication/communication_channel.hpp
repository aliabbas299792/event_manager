#ifndef COMMUNICATION_CHANNEL_
#define COMMUNICATION_CHANNEL_

#include "request_types.hpp"
#include "response_types.hpp"
#include <optional>
#include <variant>

class CommunicationChannel {
private:
  RequestVariant request_store_{std::monostate{}};
  ResponseVariant response_store_{std::monostate{}};

public:
  template <ResponseType Rt, typename RespType = RespTypeMap<Rt>>
  CommunicationChannel &set_resp_data(RespType &&data) {
    constexpr const int ItemIndex = static_cast<std::size_t>(Rt);
    response_store_.emplace<ItemIndex>(std::forward<RespType>(data));
    return *this;
  }

  template <ResponseType Rt, typename RespType = RespTypeMap<Rt>>
  [[nodiscard("Response data shouldn't be discarded")]] std::optional<RespType>
  get_resp_data() {
    constexpr const int ItemIndex = static_cast<std::size_t>(Rt);
    if (auto data = std::get_if<ItemIndex>(&response_store_)) {
      response_store_.emplace<std::monostate>();
      return std::optional<RespType>(*data);
    } else {
      return std::nullopt;
    }
  }

  template <RequestType Rt, typename ReqType = ReqTypeMap<Rt>>
  CommunicationChannel &set_req_data(ReqType &&data) {
    constexpr const int ItemIndex = static_cast<std::size_t>(Rt);
    request_store_.emplace<ItemIndex>(std::forward<ReqType>(data));
    return *this;
  }

  template <RequestType Rt, typename ReqType = ReqTypeMap<Rt>>
  [[nodiscard("Request data shouldn't be discarded")]] std::optional<ReqType>
  get_req_data() {
    constexpr const int ItemIndex = static_cast<std::size_t>(Rt);
    if (auto data = std::get_if<ItemIndex>(&request_store_)) {
      request_store_.emplace<std::monostate>();
      return std::optional<ReqType>(*data);
    } else {
      return std::nullopt;
    }
  }

  RequestType request_store_current_type() {
    return static_cast<RequestType>(request_store_.index());
  }

  ResponseType response_store_current_type() {
    return static_cast<ResponseType>(response_store_.index());
  }
};

#endif