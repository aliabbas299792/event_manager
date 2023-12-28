#include "event_manager.hpp"
#include <dirent.h>
#include <fcntl.h>

/*
Make a file test.txt in the parent file,
then this program will read the file,
print the output, then delete the file
*/

EvTask example(EventManager *ev) {
  DIR *dir = opendir("../");
  int dfd = dirfd(dir);
  auto resp = co_await ev->openat(dfd, "test.txt", O_RDWR, 0);
  auto fd = resp.data.fd;

  char buff[2048]{};
  co_await ev->read(fd, reinterpret_cast<uint8_t *>(buff), 2048);

  std::cout << buff << "\n";

  co_await ev->close(fd);

  co_await ev->unlinkat(dfd, "test.txt", 0);

  co_await ev->kill();
  closedir(dir);
  co_return 0;
}

int main() {
  EventManager ev(10);
  auto coro = example(&ev);
  ev.register_coro(&coro);
  ev.start();
  std::cout << "We're at the end of the program\n";
}