#ifndef COMMUNICATION_CHANNEL_
#define COMMUNICATION_CHANNEL_

#include "response_types.hpp"
#include <iostream>
#include <optional>
#include <variant>

class CommunicationChannel {
private:
  ResponseVariant response_store_{std::monostate{}};

public:
  template <ResponseType Rt, typename RespType = RespTypeMap<Rt>>
  CommunicationChannel &set_resp_data(RespType &&data) {
    constexpr const int ItemIndex = static_cast<std::size_t>(Rt);
    // std::cout << "current index we're putting in is " << ItemIndex << "\n";
    response_store_.emplace<ItemIndex>(std::forward<RespType>(data));
    // std::cout << "do we have the value or not: " << (std::get_if<ItemIndex>(&response_store_) == nullptr ? "no" : "yes") << "\n";
    return *this;
  }

  template <ResponseType Rt, typename RespType = RespTypeMap<Rt>>
  [[nodiscard("Response data shouldn't be discarded")]] std::optional<RespType>
  get_resp_data() {
    // std::cout << "we did get resp data\n";
    constexpr const int ItemIndex = static_cast<std::size_t>(Rt);
    // std::cout << "do we have the value or not: " << (std::get_if<ItemIndex>(&response_store_) == nullptr ? "no" : "yes") << "\n";
    if (auto data = std::get_if<ItemIndex>(&response_store_)) {
      auto ret_data = std::optional<RespType>(*data);
      response_store_.emplace<std::monostate>();
      return ret_data;
    } else {
      // std::cout << "so we apparently dont have a value<----------\n";
      return std::nullopt;
    }
  }

  ResponseType response_store_current_type() {
    return static_cast<ResponseType>(response_store_.index());
  }
};

#endif