#include "coroutine/io_awaitables.hpp"
#include "event_manager.hpp"
#include <cstring>
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
  std::string file_name = "test.txt";
  auto resp = co_await ev->openat(dfd, file_name.c_str(), O_RDWR, 0);

    
  if(resp.data.error_num != 0) {
    std::cerr << "There was an error in opening the file " << file_name << " in the parent directory: " << strerror(resp.data.error_num) << "\n";
    co_await ev->kill();
    co_return -1;
  }
  
  auto fd = resp.data.req_fd;

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
  // register it with the system, which will run it once it has started
  ev.register_coro(example, &ev);
  ev.start();
  std::cout << "We're at the end of the program\n";
}