#include "coroutine/io_awaitables.hpp"
#include "errors.hpp"
#include "event_loop/event_manager.hpp"
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

constexpr const size_t BACKLOG = 10;

void fatal_error(std::string error_message) {
  perror(std::string("Fatal Error: " + error_message).c_str());
  exit(1);
}

int setup_listener(uint16_t port) {
  int listener_fd;
  uint8_t yes = 1;
  addrinfo hints, *server_info, *traverser;

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // tcp
  hints.ai_flags = AI_PASSIVE;     // use local IP

  if (getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &server_info) != 0)
    fatal_error("getaddrinfo");

  for (traverser = server_info; traverser != NULL; traverser = traverser->ai_next) {
    if ((listener_fd = socket(traverser->ai_family, traverser->ai_socktype, traverser->ai_protocol)) ==
        -1) // ai_protocol may be usefulin the future I believe, only UDP/TCP right now, may
      fatal_error("socket construction");

    if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) ==
        -1) // 2nd param (SOL_SOCKET) is saying to do it at the socket protocol level, not TCP or anything
            // else, just for the socket
      fatal_error("setsockopt SO_REUSEADDR");

    if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) == -1)
      fatal_error("setsockopt SO_REUSEPORT");

    if (setsockopt(listener_fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1)
      fatal_error("setsockopt SO_KEEPALIVE");

    size_t keep_idle = 1000; // The time (in seconds) the connection needs to remain idle before TCP starts
                             // sending keepalive probes, if the socket option SO_KEEPALIVE has been set on
                             // this socket.  This option should not be used in code intended to be portable.
    if (setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle)) == -1)
      fatal_error("setsockopt TCP_KEEPIDLE");

    size_t keep_interval = 1000; // The time (in seconds) between individual keepalive probes. This option
                                 // should not be used in code intended to be portable.
    if (setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval)) == -1)
      fatal_error("setsockopt TCP_KEEPINTVL");

    size_t keep_count = 10; // The maximum number of keepalive probes TCP should send before dropping the
                            // connection.  This option should not be used in code intended to be portable.
    if (setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPCNT, &keep_count, sizeof(keep_count)) == -1)
      fatal_error("setsockopt TCP_KEEPCNT");

    if (bind(listener_fd, traverser->ai_addr, traverser->ai_addrlen) ==
        -1) { // try to bind the socket using the address data supplied, has internet address, address family
              // and port in the data
      perror("bind");
      continue; // not fatal, we can continue
    }

    break; // we got here, so we've got a working socket - so break
  }

  freeaddrinfo(server_info); // free the server_info linked list

  if (traverser == NULL) // means we didn't break, so never got a socket made successfully
    fatal_error("no socket made");

  if (listen(listener_fd, BACKLOG) == -1)
    fatal_error("listen");

  return listener_fd;
}

EvTask send_hello_world(EventManager *ev, int user_fd) {
  std::string headers_p1 = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: ";
  std::string headers_p2 = "\r\n\r\n";
  std::string content = "Hello World!\r\n";
  std::string data = headers_p1 + std::to_string(content.length()) + headers_p2 + content;
  std::cout << data.data() << "\nwas the buffer\n";

  auto write_resp = co_await ev->write(user_fd, reinterpret_cast<uint8_t *>(data.data()), data.length());

  if (isThereAnError(write_resp.error)) {
    std::cerr << "There was an error in handling the request for fd " << user_fd << "\n";
    co_return -1;
  }

  auto close_resp = co_await ev->close(user_fd);

  if (isThereAnError(close_resp.error)) {
    std::cerr << "There was an error in handling the close request for fd " << user_fd << "\n";
    std::string err_str{};
    size_t error_number{};
    switch (getErrorType(close_resp.error)) {
    case ErrorType::NO_ERR:
      break;
    case ErrorType::EVENT_MANAGER_ERR: {
      err_str = "There was an error with the event manager: ";
      auto err_num = getContainedErrorCode<ErrorType::EVENT_MANAGER_ERR>(close_resp.error)
                         .value_or(EventManagerErrors::UNKNOWN_ERROR);
      error_number = static_cast<size_t>(err_num);
      switch (err_num) {
      case EventManagerErrors::UNKNOWN_ERROR:
        err_str += "and we don't know what the error was\n";
        break;
      case EventManagerErrors::SUBMISSION_QUEUE_FULL:
        err_str += "the submission queue was full\n";
        break;
      case EventManagerErrors::SYSTEM_COMMUNICATION_CHANNEL_FAILURE:
        err_str += "the communication channels have failed us\n";
        break;
      }
      break;
    }
    case ErrorType::LIBURING_SUBMISSION_ERR_ERRNO: {
      // default to a permission error
      auto err_num = getContainedErrorCode<ErrorType::LIBURING_SUBMISSION_ERR_ERRNO>(close_resp.error)
                         .value_or(Errnos::UNKNOWN_ERROR);
      error_number = static_cast<size_t>(err_num);
      err_str = strerror(static_cast<int>(err_num));
      break;
    }
    case ErrorType::OPERATION_ERR_ERRNO: {
      // default to a permission error
      auto err_num = getContainedErrorCode<ErrorType::OPERATION_ERR_ERRNO>(close_resp.error)
                         .value_or(Errnos::UNKNOWN_ERROR);
      error_number = static_cast<size_t>(err_num);
      err_str = strerror(static_cast<int>(err_num));
      break;
    }
    }

    std::cerr << err_str << "\n";
    co_return error_number;
  }

  co_return 0;
}

EvTask coro(EventManager *ev) {
  int listener_fd = setup_listener(3050);

  while (true) {
    sockaddr addr{};
    socklen_t addrlen = sizeof(addr);
    int fd = (co_await ev->accept(listener_fd, &addr, &addrlen)).data.fd;

    ev->register_coro(send_hello_world, ev, fd);
  }

  co_await ev->close(listener_fd);
  co_await ev->kill();
  co_return 0;
}

void signal_callback_handler(int signum) { printf("Caught signal SIGPIPE %d\n", signum); }

int main() {
  signal(SIGPIPE, signal_callback_handler);
  const size_t queue_depth =
      10; // i.e how many items may be in the internal queue before it needs to be flushed, max is 4096
  std::cout << "starting program\n";
  EventManager ev{queue_depth};

  // register it with the system, which will run it once it has started
  ev.register_coro(coro(&ev));
  // start it
  ev.start();
}
