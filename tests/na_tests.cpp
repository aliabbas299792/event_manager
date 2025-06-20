#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "vendor/doctest/doctest/doctest.h"

#include "event_loop/event_manager.hpp"
#include <fcntl.h>
#include <string>
#include <unistd.h>

EvTask na_coro(EventManager* ev, int fd, std::string write_data, std::string& read_out) {
  ev->write_na(fd, reinterpret_cast<const uint8_t*>(write_data.c_str()), write_data.size());
  char buffer[128]{};
  co_await ev->poll([&](EventManager* ev, RequestType type, CommunicationChannel* channel) {
    if (type == RequestType::WRITE) {
      channel->consume_resp_data<RequestType::WRITE>();
      lseek(fd, 0, SEEK_SET);
      ev->read_na(fd, reinterpret_cast<uint8_t*>(buffer), sizeof(buffer));
    } else if (type == RequestType::READ) {
      auto data = channel->consume_resp_data<RequestType::READ>();
      read_out.assign(buffer, data->bytes_read);
      ev->close_na(fd);
    } else if (type == RequestType::CLOSE) {
      channel->consume_resp_data<RequestType::CLOSE>();
      return PollingState::STOP_POLLING;
    }
    return PollingState::CONTINUE_POLLING;
  });
  co_await ev->kill();
  co_return 0;
}

TEST_CASE("Non awaitable operations do not leak memory") {
  const char* path = "./na_test.txt";
  int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
  REQUIRE(fd >= 0);
  std::string result;
  {
    EventManager ev(10);
    ev.register_coro(na_coro(&ev, fd, "hello", result));
    ev.start();
  }
  REQUIRE(result == "hello");
  close(fd);
  unlink(path);
}
