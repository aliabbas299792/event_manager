#ifndef COMMUNICATION_CHANNEL_
#define COMMUNICATION_CHANNEL_

#include "communication_types.hpp"
#include <iostream>
#include <optional>
#include <variant>

class CommunicationChannel {
private:
  ResponseVariant response_store_{std::monostate{}};

public:
  template <RequestType Rt, typename RespType = RespDataTypeMap<Rt>>
  CommunicationChannel &publish_resp_data(RespType &&data) {
    constexpr const int ItemIndex = static_cast<std::size_t>(Rt);
    response_store_.emplace<ItemIndex>(std::forward<RespType>(data));
    return *this;
  }

  template <RequestType Rt, typename RespType = RespDataTypeMap<Rt>>
  [[nodiscard("Response data shouldn't be discarded")]] std::optional<RespType>
  consume_resp_data() {
    constexpr const int ItemIndex = static_cast<std::size_t>(Rt);
    if (auto data = std::get_if<ItemIndex>(&response_store_)) {
      auto ret_data = std::optional<RespType>(*data);
      response_store_.emplace<std::monostate>();
      return ret_data;
    } else {
      return std::nullopt;
    }
  }

  RequestType response_store_current_type() {
    return static_cast<RequestType>(response_store_.index());
  }
};

#endif