#include "coroutine/io_awaitables.hpp"
#include "errors.hpp"
#include <cstring>
#include <dirent.h>
#include <fcntl.h>

/*
Make a file test.txt in the parent file,
then this program will read the file,
print the output, then delete the file
*/

EvTask example(EventManager *ev) {
  using namespace ErrorProcessing;
  
  DIR *dir = opendir("../");
  int dfd = dirfd(dir);
  std::string file_name = "test.txt";
  auto resp = co_await ev->openat(dfd, file_name.c_str(), O_RDWR, 0);

  if (is_there_an_error(resp.error)) {
    std::string err_str{};
    switch (get_error_type(resp.error)) {
    case ErrorType::NO_ERR:
      break;
    case ErrorType::EVENT_MANAGER_ERR: {
      err_str = "There was an error with the event manager";
      break;
    }
    case ErrorType::LIBURING_SUBMISSION_ERR_ERRNO: {
      auto err_num = get_contained_error_code<ErrorType::LIBURING_SUBMISSION_ERR_ERRNO>(resp.error)
                         .value_or(Errnos::UNKNOWN_ERROR);
      err_str = strerror(static_cast<int>(err_num));
      break;
    }
    case ErrorType::OPERATION_ERR_ERRNO: {
      auto err_num = get_contained_error_code<ErrorType::OPERATION_ERR_ERRNO>(resp.error)
                         .value_or(Errnos::UNKNOWN_ERROR);
      err_str = strerror(static_cast<int>(err_num));
      break;
    }
    }
    std::cerr << "There was an error in opening the file " << file_name
              << " in the parent directory: " << strerror(resp.data.error_num) << "\n";
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