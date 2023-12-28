#include "event_manager.hpp"
#include <fcntl.h>

EvTask coro(EventManager *ev) {
  // open a file and make a buffer to read into
  int fd = open("test.txt", O_RDWR);
  constexpr int size = 2048;
  char buff[size]{};

  // read some data and print it
  co_await ev->read(fd, reinterpret_cast<uint8_t*>(buff), size);
  std::cout << "Read:\n" << buff << "\n";

  // close the file, and kill the event manager
  co_await ev->close(fd);
  co_await ev->kill();
  co_return 0;
}

int main() {
  const int queue_depth = 10; // i.e how many items may be in the internal queue before it needs to be flushed, max is 4096
  EventManager ev{queue_depth};

  // make the coroutine task
  auto task = coro(&ev);
  // register it with the system, which will run it once it has started
  ev.register_coro(&task);
  // start it
  ev.start();
}