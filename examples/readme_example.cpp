#include "event_manager.hpp"
#include <cstdio>
#include <fcntl.h>

EvTask coro(EventManager *ev) {
  // open a file and make a buffer to read into
  std::string file_name = "test.txt";
  int fd = open(file_name.c_str(), O_RDWR);

  if(fd < 0) {
    std::cerr << "There was an error in opening the file " << file_name << ": (" << errno << ") ";
    perror("");
    co_await ev->kill();
    co_return -1;
  }

  constexpr size_t size = 2048;
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
  const size_t queue_depth = 10; // i.e how many items may be in the internal queue before it needs to be flushed, max is 4096
  EventManager ev{queue_depth};

  // register it with the system, which will run it once it has started
  ev.register_coro(coro, &ev);
  // start it
  ev.start();
}