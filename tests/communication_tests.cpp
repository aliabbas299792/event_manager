#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "vendor/doctest/doctest/doctest.h"

#include "event_manager.hpp"

TEST_CASE("Testing response storing") {
  CommunicationChannel cc{};

  SUBCASE("Testing optional returns nothing for all response types") {
    // This test just checks an empty buffer can be checked for response data
    // and it's fine
    {
      auto ret = cc.consume_resp_data<RequestType::READ>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.consume_resp_data<RequestType::WRITE>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.consume_resp_data<RequestType::CLOSE>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.consume_resp_data<RequestType::SHUTDOWN>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.consume_resp_data<RequestType::READV>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.consume_resp_data<RequestType::WRITEV>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.consume_resp_data<RequestType::ACCEPT>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.consume_resp_data<RequestType::CONNECT>();
      REQUIRE(!ret.has_value());
    }
  }

  SUBCASE("Testing storing data and then getting it") {
    // This test checks if 1) storing the value works 2) retrieving it works and
    // 3) the buffer is emptied after retrieval
    {
      constexpr auto Rt = RequestType::READ;

      RespDataTypeMap<Rt> default_value{};
      cc.publish_resp_data<Rt>(default_value);
      auto ret = cc.consume_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value().bytes_read == default_value.bytes_read);
      REQUIRE(!cc.consume_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::WRITE;

      RespDataTypeMap<Rt> default_value{};
      cc.publish_resp_data<Rt>(default_value);
      auto ret = cc.consume_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value().bytes_wrote == default_value.bytes_wrote);
      REQUIRE(!cc.consume_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::CLOSE;

      RespDataTypeMap<Rt> default_value{};
      cc.publish_resp_data<Rt>(default_value);
      auto ret = cc.consume_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value().error_num == default_value.error_num);
      REQUIRE(!cc.consume_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::SHUTDOWN;

      RespDataTypeMap<Rt> default_value{};
      cc.publish_resp_data<Rt>(default_value);
      auto ret = cc.consume_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value().error_num == default_value.error_num);
      REQUIRE(!cc.consume_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::READV;

      RespDataTypeMap<Rt> default_value{};
      cc.publish_resp_data<Rt>(default_value);
      auto ret = cc.consume_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value().bytes_read == default_value.bytes_read);
      REQUIRE(!cc.consume_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::WRITEV;

      RespDataTypeMap<Rt> default_value{};
      cc.publish_resp_data<Rt>(default_value);
      auto ret = cc.consume_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value().bytes_wrote == default_value.bytes_wrote);
      REQUIRE(!cc.consume_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::ACCEPT;

      RespDataTypeMap<Rt> default_value{};
      cc.publish_resp_data<Rt>(default_value);
      auto ret = cc.consume_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value().error_num == default_value.error_num);
      REQUIRE(!cc.consume_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::CONNECT;

      RespDataTypeMap<Rt> default_value{};
      cc.publish_resp_data<Rt>(default_value);
      auto ret = cc.consume_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value().error_num == default_value.error_num);
      REQUIRE(!cc.consume_resp_data<Rt>().has_value());
    }
  }
}