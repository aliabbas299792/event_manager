#include "communication/communication_channel.hpp"
#include "coroutine/task.hpp"
#include "coroutine/awaiter.hpp"
#include <coroutine>
#include <iostream>
#include <variant>

void visit_request_types(CommunicationChannel &cc) {
  switch (cc.request_store_current_type()) {
  case RequestType::READ:
    std::cout << "it was a read request: " << cc.get_req_data<RequestType::READ>().value() << "\n";
    break;
  case RequestType::WRITE:
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
    auto val = x.value() + myval;
    std::cout << "got back " << val << "\n";
  }

  co_return;
};

int main() {
  auto coro = test_coro();
  
  auto a = coro.start();
  visit_request_types(a);

  auto b = coro.resume<ResponseType::READ>(53.5);

  std::cout << "Hello world\n";
}