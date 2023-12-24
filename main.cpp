#include "communication/communication_channel.hpp"
#include "communication/communication_types.hpp"
#include "coroutine/io_awaitables.hpp"
#include "coroutine/task.hpp"
#include "event_loop/event_manager.hpp"
#include <coroutine>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <thread>
#include <variant>

EvTask other_coro_read(EventManager *ev, int fd, uint8_t *buff) {
  auto res = co_await ev->read(fd, buff, 2048);
  co_return res.data.bytes_read;
}

EvTask other_coro_write(EventManager *ev, int fd, std::string &str) {
  auto res = co_await ev->write(fd, reinterpret_cast<uint8_t *>(str.data()),
                                str.length());
  co_return res.data.bytes_wrote;
}

EvTask coro(EventManager *ev) {
  int fd = open("../test.txt", O_RDWR);
  uint8_t buff[2048]{};

  co_await ev->read(fd, buff, 2048);
  std::cout << "Read this data:\n--------START--------\n";
  std::cout << buff << "\n---------END---------\n";

  co_await ev->close(fd);
  fd = open("../test.txt", O_RDWR | O_TRUNC);
  std::cout << "The new fd is " << fd << "\n\n";

  std::string str = "Hello world this is a message from the program";
  std::cout << "Writing: \"" << str << "\" to the file instead\n";
  auto res = co_await ev->write(fd, reinterpret_cast<uint8_t *>(str.data()),
                                str.length());
  std::cout << "How many bytes were written: " << res.data.bytes_wrote << "\n";

  std::memset(buff, 0, 2048);
  co_await ev->read(fd, buff, 2048);
  std::cout << "Read this data:\n--------START--------\n";
  std::cout << buff << "\n---------END---------\n";

  co_await ev->close(fd);

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  co_await ev->kill();

  std::cout << "here we are after it is dead\n";
  co_return 0;
}

int main() {
  while (true) {
    EventManager ev(10);
    auto coroTask = coro(&ev);

    ev.register_coro(coroTask);
    ev.start();

    std::cout << "->>>>>>>>>>>>>>>> Completed an iteration\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}