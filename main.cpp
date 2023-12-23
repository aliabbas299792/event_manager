#include "communication/communication_channel.hpp"
#include "communication/response_types.hpp"
#include "coroutine/task.hpp"
#include "event_loop/event_manager.hpp"
#include <coroutine>
#include <fcntl.h>
#include <iostream>
#include <variant>

EvTask other_coro_read(EventManager *ev, int fd, uint8_t *buff) {
  auto res = co_await ev->read(fd, buff, 2048);
  co_return res.data.bytes_read;
}

EvTask other_coro_write(EventManager *ev, int fd, std::string &str) {
  auto res = co_await ev->write(fd, reinterpret_cast<uint8_t*>(str.data()), str.length());
  std::cout << res.data.bytes_wrote << " we are writing\n";
  co_return res.data.bytes_wrote;
}

EvTask coro(EventManager *ev) {
  int fd = open("../test.txt", O_RDWR);
  std::cout << fd << " is the fd\n";
  uint8_t buff[2048]{};

  auto res = co_await ev->read(fd, buff, 2048);
  std::cout << "res is the ressonse " << res.data.bytes_read << "\n";

  std::string str = "hello world this is a message from the program this is epic\n";
  auto res2 = co_await ev->write(fd, reinterpret_cast<uint8_t*>(str.data()), str.length());
  std::cout << "we co awaited\n";
  std::cout << "res2 " << res2.data.bytes_wrote << "\n";
  co_return 0;
}

int main() {
  EventManager ev(10);
  ev.register_coro(coro(&ev));
  ev.start();
  std::cout << "Hello world\n";
}