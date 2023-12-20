#include "request_types.hpp"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "vendor/doctest/doctest.h"

#include "communication_channel.hpp"

TEST_CASE("Testing request storing") {
  CommunicationChannel cc{};

  SUBCASE("Testing optional returns nothing for all request types") {
    // This test just checks an empty buffer can be checked for request data and
    // it's fine
    {
      auto ret = cc.get_req_data<RequestType::READ>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_req_data<RequestType::WRITE>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_req_data<RequestType::OPEN>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_req_data<RequestType::CLOSE>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_req_data<RequestType::SHUTDOWN>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_req_data<RequestType::READV>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_req_data<RequestType::WRITEV>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_req_data<RequestType::ACCEPT>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_req_data<RequestType::CONNECT>();
      REQUIRE(!ret.has_value());
    }
  }

  SUBCASE("Testing storing data and then getting it") {
    // This test checks if 1) storing the value works 2) retrieving it works and
    // 3) the buffer is emptied after retrieval
    {
      constexpr auto Rt = RequestType::READ;

      ReqTypeMap<Rt> default_value{};
      cc.set_req_data<Rt>(default_value);
      auto ret = cc.get_req_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_req_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::WRITE;

      ReqTypeMap<Rt> default_value{};
      cc.set_req_data<Rt>(default_value);
      auto ret = cc.get_req_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_req_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::OPEN;

      ReqTypeMap<Rt> default_value{};
      cc.set_req_data<Rt>(default_value);
      auto ret = cc.get_req_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_req_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::CLOSE;

      ReqTypeMap<Rt> default_value{};
      cc.set_req_data<Rt>(default_value);
      auto ret = cc.get_req_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_req_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::SHUTDOWN;

      ReqTypeMap<Rt> default_value{};
      cc.set_req_data<Rt>(default_value);
      auto ret = cc.get_req_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_req_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::READV;

      ReqTypeMap<Rt> default_value{};
      cc.set_req_data<Rt>(default_value);
      auto ret = cc.get_req_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_req_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::WRITEV;

      ReqTypeMap<Rt> default_value{};
      cc.set_req_data<Rt>(default_value);
      auto ret = cc.get_req_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_req_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::ACCEPT;

      ReqTypeMap<Rt> default_value{};
      cc.set_req_data<Rt>(default_value);
      auto ret = cc.get_req_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_req_data<Rt>().has_value());
    }
    {
      constexpr auto Rt = RequestType::CONNECT;

      ReqTypeMap<Rt> default_value{};
      cc.set_req_data<Rt>(default_value);
      auto ret = cc.get_req_data<Rt>();

      REQUIRE(ret.has_value());
      REQUIRE(ret.value() == default_value);
      REQUIRE(!cc.get_req_data<Rt>().has_value());
    }
  }
}

TEST_CASE("Testing response storing") {
  CommunicationChannel cc{};

  SUBCASE("Testing optional returns nothing for all response types") {
    // This test just checks an empty buffer can be checked for response data and
    // it's fine
    {
      auto ret = cc.get_resp_data<ResponseType::READ>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_resp_data<ResponseType::WRITE>();
      REQUIRE(!ret.has_value());
    }
    {
      auto ret = cc.get_resp_data<ResponseType::OPEN>();
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
      constexpr auto Rt = ResponseType::OPEN;

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