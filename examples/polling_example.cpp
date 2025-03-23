#include "event_loop/event_manager.hpp"
#include <fcntl.h>
#include <thread>

EvTask coro(EventManager* ev) {
  int fd = open("example.txt", O_RDWR | O_CREAT);

  int counter = 0;
  std::string write_data = "Hello, World! Counter is: " + std::to_string(counter);
  ev->write_na(fd, reinterpret_cast<const uint8_t*>(write_data.c_str()), sizeof(write_data));

  char buffer[1024]{};
  co_await ev->poll([&](EventManager* ev, RequestType type, CommunicationChannel* channel) {
    if (type == RequestType::READ) {
      auto data = channel->consume_resp_data<RequestType::READ>();
      std::cout << "Read " << data->bytes_read << " bytes\n";
      std::cout << "Buffer:\n---\n" << data->buff << "\n---\n";

      counter++;
      write_data = "Hello, World! Counter is: " + std::to_string(counter);
      ev->write_na(fd, reinterpret_cast<const uint8_t*>(write_data.c_str()), sizeof(data));

    } else if (type == RequestType::WRITE) {
      auto data = channel->consume_resp_data<RequestType::WRITE>();
      std::cout << "Wrote " << data->bytes_wrote << " bytes\n";
      ev->read_na(fd, reinterpret_cast<uint8_t*>(buffer), 1024);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "\n\n";

    return PollingState::CONTINUE_POLLING;
  });

  co_await ev->kill();
  co_return 0;
}

void signal_callback_handler(int signum) {
  printf("Caught signal SIGPIPE %d\n", signum);
}

int main() {
  signal(SIGPIPE, signal_callback_handler);
  const size_t QUEUE_DEPTH =
      10;  // i.e how many items may be in the internal queue before it needs to be flushed, max is 4096
  std::cout << "starting program\n";
  EventManager ev{QUEUE_DEPTH};

  // register it with the system, which will run it once it has started
  ev.register_coro(coro(&ev));
  // start it
  ev.start();
}
