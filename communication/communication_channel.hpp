#ifndef COMMUNICATION_CHANNEL_
#define COMMUNICATION_CHANNEL_

#include "communication_types.hpp"
#include <optional>
#include <variant>

class CommunicationChannel {
private:
  ResponseVariant _response_store{std::monostate{}};

public:
  template <RequestType Rt, typename RespType = RespDataTypeMap<Rt>>
  CommunicationChannel& publish_resp_data(RespType&& data) {
    constexpr const auto ITEM_INDEX = static_cast<size_t>(Rt);
    _response_store.emplace<ITEM_INDEX>(std::forward<RespType>(data));
    return *this;
  }

  template <RequestType Rt, typename RespType = RespDataTypeMap<Rt>>
  [[nodiscard("Response data shouldn't be discarded")]] std::optional<RespType> consume_resp_data() {
    constexpr const auto ITEM_INDEX = static_cast<size_t>(Rt);
    if (auto data = std::get_if<ITEM_INDEX>(&_response_store)) {
      auto ret_data = std::optional<RespType>(*data);
      _response_store.emplace<std::monostate>();
      return ret_data;
    } else {
      return std::nullopt;
    }
  }

  RequestType response_store_current_type() {
    return static_cast<RequestType>(_response_store.index());
  }
};

#endif