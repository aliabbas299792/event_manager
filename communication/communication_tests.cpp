#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "vendor/doctest/doctest.h"

#include "communication_channel.hpp"

TEST_CASE("Testing response storing") {
  CommunicationChannel cc{};

  SUBCASE("Testing optional returns nothing for all response types") {
    // This test just checks an empty buffer can be checked for response data
    // and it's fine
    {
      auto ret = cc.get_resp_data<ResponseType::READ>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_resp_data<ResponseType::WRITE>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_resp_data<ResponseType::CLOSE>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_resp_data<ResponseType::SHUTDOWN>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_resp_data<ResponseType::READV>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_resp_data<ResponseType::WRITEV>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_resp_data<ResponseType::ACCEPT>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_resp_data<ResponseType::CONNECT>();
      REQUIRE(!ret.has_value());
    }
  }

  SUBCASE("Testing storing data and then getting it") {
    // This test checks if 1) storing the value works 2) retrieving it works and
    // 3) the buffer is emptied after retrieval
    {
      constexpr auto Rt = ResponseType::READ;

      RespTypeMap<Rt> default_value{};
      cc.set_resp_data<Rt>(default_value);
      auto ret = cc.get_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = ResponseType::WRITE;

      RespTypeMap<Rt> default_value{};
      cc.set_resp_data<Rt>(default_value);
      auto ret = cc.get_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = ResponseType::CLOSE;

      RespTypeMap<Rt> default_value{};
      cc.set_resp_data<Rt>(default_value);
      auto ret = cc.get_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = ResponseType::SHUTDOWN;

      RespTypeMap<Rt> default_value{};
      cc.set_resp_data<Rt>(default_value);
      auto ret = cc.get_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = ResponseType::READV;

      RespTypeMap<Rt> default_value{};
      cc.set_resp_data<Rt>(default_value);
      auto ret = cc.get_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = ResponseType::WRITEV;

      RespTypeMap<Rt> default_value{};
      cc.set_resp_data<Rt>(default_value);
      auto ret = cc.get_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = ResponseType::ACCEPT;

      RespTypeMap<Rt> default_value{};
      cc.set_resp_data<Rt>(default_value);
      auto ret = cc.get_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_resp_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = ResponseType::CONNECT;

      RespTypeMap<Rt> default_value{};
      cc.set_resp_data<Rt>(default_value);
      auto ret = cc.get_resp_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_resp_data<Rt>().has_value());
    }
  }
}