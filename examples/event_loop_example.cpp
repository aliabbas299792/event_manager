#include "event_manager.hpp"
#include <cstring>
#include <fcntl.h>
#include <sstream>

const std::string lorem_ipsum = R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean ultricies
ex sit amet orci tincidunt, a viverra sem suscipit. Phasellus non quam
bibendum, molestie enim vel, placerat nibh. Mauris elementum, nisi et
fringilla auctor, velit justo porttitor neque, ut viverra neque magna
quis urna. Mauris eu libero lacinia, pulvinar purus nec, dapibus mi.
Aliquam id accumsan nunc, vitae molestie diam. Proin eget aliquet orci,
vitae pretium ante. Proin libero augue, vulputate eget iaculis vel,
ultricies at velit. Phasellus euismod nisl vitae lacus scelerisque
imperdiet sit amet vitae mi. Quisque rutrum leo et placerat iaculis.
Fusce ac mattis eros, aliquet fringilla tellus. Quisque mauris mauris,
dapibus nec orci et, lobortis tincidunt mauris.)";

std::stringstream string_output{};
#define OUTPUT string_output
// #define OUTPUT std::cout

const uint8_t *get_write_data(const std::string &str) {
  return reinterpret_cast<const uint8_t *>(str.data());
}

std::string indent_with_str(const std::string &str, const std::string &indent) {
  std::string out{indent};
  for (const char c : str) {
    out += c;
    if (c == '\n') {
      out += indent;
    }
  }
  return out;
}

EvTask coro4() { co_return 2; }

EvTask coro3() { co_return co_await coro4(); }

EvTask coro2() { co_return co_await coro3(); }

EvTask coro1() { co_return co_await coro2(); }

int num_currently_being_processed{};

EvTask coro(EventManager *ev) {
  std::cout << "at start\n";
  std::cout << co_await coro1() << " (coro1 response)\n";
  std::cout << "just after that\n";
  auto filepath = "./test.txt";

  int fd = open(filepath, O_RDWR | O_CREAT);

  char buff[2048]{};
  co_await ev->write(fd, get_write_data(lorem_ipsum), lorem_ipsum.length());

  co_await ev->read(fd, reinterpret_cast<uint8_t *>(buff), 2048);
  OUTPUT << "(*) Read this data:\n" << indent_with_str(buff, "> ") << "\n";
  std::cout << co_await coro2() << " (coro2 response)\n";

  int fd1 = open("example1.txt", O_RDWR | O_CREAT, 0666);
  int fd2 = open("example2.txt", O_RDWR | O_CREAT, 0666);
  int fd3 = open("example3.txt", O_RDWR | O_CREAT, 0666);
  int fd4 = open("example4.txt", O_RDWR | O_CREAT, 0666);
  int fd5 = open("example5.txt", O_RDWR | O_CREAT, 0666);
  auto queue = ev->make_request_queue();
  queue.queue_write(fd1, get_write_data(lorem_ipsum), lorem_ipsum.length());
  queue.queue_write(fd2, get_write_data(lorem_ipsum), lorem_ipsum.length());
  queue.queue_write(fd3, get_write_data(lorem_ipsum), lorem_ipsum.length());
  queue.queue_write(fd4, get_write_data(lorem_ipsum), lorem_ipsum.length());
  queue.queue_write(fd5, get_write_data(lorem_ipsum), lorem_ipsum.length());

  co_await ev->submit_and_wait(queue, [](RequestType req_type, CommunicationChannel *channel) {
    switch (req_type) {
    case RequestType::READ: {
      break;
    }
    case RequestType::WRITE: {
      auto data = channel->consume_resp_data<RequestType::WRITE>();
      std::cout << "[main] Wrote " << data->bytes_wrote << " bytes for fd " << data->req_fd << "\n";
      break;
    }
    case RequestType::CLOSE: {
      break;
    }
    case RequestType::SHUTDOWN: {
      break;
    }
    case RequestType::READV: {
      break;
    }
    case RequestType::WRITEV: {
      break;
    }
    case RequestType::ACCEPT: {
      break;
    }
    case RequestType::CONNECT: {
      break;
    }
    case RequestType::OPENAT: {
      break;
    };
    case RequestType::STATX: {
      break;
    };
    case RequestType::UNLINKAT: {
      break;
    };
    case RequestType::RENAMEAT: {
      break;
    };
    }
  });

  std::cout << "after submit and wait\n";

  std::cout << co_await coro3() << " (coro3 response)\n";
  std::memset(buff, 0, 2048);
  co_await ev->read(fd, reinterpret_cast<uint8_t *>(buff), 2048);
  OUTPUT << "(*) Read this data:\n" << indent_with_str(buff, "> ") << "\n\n";

  if (--num_currently_being_processed == 0) {
    std::cout << "We killed the event manager\n";
    co_await ev->kill();
  }
  std::cout << num_currently_being_processed << " is num being processed\n";

  std::cout << "we're at the end\n";
  co_return 0;
}

EvTask do_thing(EventManager *ev) {
  auto task1 = coro(ev);
  auto task2 = coro(ev);
  co_await task1;
  co_await task2;
  co_return 4;
}

int main() {
  EventManager ev(10);
  num_currently_being_processed = 5; // we intend to set off 5 coroutines

  ev.register_coro(coro, &ev);
  ev.register_coro(coro, &ev);
  ev.register_coro(coro(&ev));
  ev.register_coro(coro(&ev));

  auto coroTask = coro(&ev);
  ev.register_coro(std::move(coroTask));
  ev.start();

  std::cout << "We're at the end of the program\n";
}