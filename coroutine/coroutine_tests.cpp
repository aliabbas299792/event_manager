#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "vendor/doctest/doctest.h"

#include "awaiter.hpp"
#include "task.hpp"

std::vector<std::string> global_log{};

void visit_request_types(CommunicationChannel *channel) {
  if (!channel) {
    std::cerr << "Cannot visit types in channel since it's a nullptr\n";
    return;
  }

  switch (channel->request_store_current_type()) {
  case RequestType::READ: {
    std::string str = "it was a read request: ";
    str +=
        std::to_string((int)channel->get_req_data<RequestType::READ>().value());
    global_log.push_back(str);
    break;
  }
  case RequestType::WRITE: {
    std::string str = "it was a write request: ";
    str += std::to_string(
        (int)channel->get_req_data<RequestType::WRITE>().value());
    global_log.push_back(str);
    break;
  }
  case RequestType::OPEN:
  case RequestType::CLOSE:
  case RequestType::SHUTDOWN:
  case RequestType::READV:
  case RequestType::WRITEV:
  case RequestType::ACCEPT:
  case RequestType::CONNECT:
    break;
  }
}

EvTask test_coro() {
  float myval = 323.2;
  auto x = co_await EvAwaiter<RequestType::READ, ResponseType::READ>{myval};
  if (x.has_value()) {
    myval += x.value();
    std::string str = "got back ";
    str += std::to_string((int)myval);
    str += " in the test coro";
    global_log.push_back(str);
  }

  co_await EvAwaiter<RequestType::WRITE, ResponseType::WRITE>(myval);
  co_return;
};

TEST_CASE("Example coroutine message passing") {
  auto coro = test_coro();

  auto a = coro.start();
  visit_request_types(a);

  auto b = coro.resume<ResponseType::READ>(53.5);
  visit_request_types(b);

  // we expect exactly this output from the back and forth
  std::vector<std::string> expected{"it was a read request: 323",
                                    "got back 376 in the test coro",
                                    "it was a write request: 376"};

  REQUIRE(global_log.size() == expected.size());

  for (size_t i = 0; i < std::min(expected.size(), global_log.size()); i++) {
    REQUIRE(global_log[i] == expected[i]);
  }
}