#ifndef TEST_FUNCTIONS
#define TEST_FUNCTIONS

#include "../event_manager.hpp"

#include <asm-generic/errno.h>
#include <cstring>
#include <thread>

#include <netdb.h>
#include <netinet/tcp.h>

const std::string text_message =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque cursus "
    "iaculis felis ut faucibus. Pellentesque sed eleifend ipsum. Aenean eget "
    "neque eu diam lobortis sodales. Nam gravida nisl in lacus convallis.";
const std::string text_message_2 =
    "Cras lorem quam, interdum sit amet sem a, congue blandit urna. Lorem "
    "ipsum dolor sit amet, consectetur adipiscing elit. Quisque cursus iaculis "
    "felis ut faucibus. Pellentesque sed eleifend ipsum. Vivamus ac feugiat "
    "quam. Vivamus venenatis auctor neque vel lacinia. Nulla lorem ipsum, "
    "ultrices sed odio vel, mattis aliquet odio. Nam suscipit in lacus eget "
    "volutpat. Cras lorem quam, interdum sit amet sem a, congue blandit urna.";
const std::string long_message =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque cursus "
    "iaculis felis ut faucibus. Pellentesque sed eleifend ipsum. Aenean eget "
    "neque eu diam lobortis sodales. Nam gravida nisl in lacus convallis, sed "
    "convallis ante rutrum. Nam facilisis massa leo, quis hendrerit nisi "
    "accumsan et. Praesent id orci nec lorem varius elementum eu eget purus. "
    "Nullam laoreet suscipit leo sit amet sodales. Mauris non nibh sit amet "
    "est faucibus ullamcorper. Maecenas imperdiet velit ut blandit semper. "
    "Proin ultrices luctus nulla eleifend pharetra. Vivamus ac feugiat quam. "
    "Vivamus venenatis auctor neque vel lacinia. Nulla lorem ipsum, ultrices "
    "sed odio vel, mattis aliquet odio. Nam suscipit in lacus eget volutpat. "
    "Cras lorem quam, interdum sit amet sem a, congue blandit urna. Lorem "
    "ipsum dolor sit amet, consectetur adipiscing elit. Quisque cursus iaculis "
    "felis ut faucibus. Pellentesque sed eleifend ipsum. Aenean eget neque eu "
    "diam lobortis sodales. Nam gravida nisl in lacus convallis, sed convallis "
    "ante rutrum. Nam facilisis massa leo, quis hendrerit nisi accumsan et. "
    "Praesent id orci nec lorem varius elementum eu eget purus. Nullam laoreet "
    "suscipit leo sit amet sodales. Mauris non nibh sit amet est faucibus "
    "ullamcorper. Maecenas imperdiet velit ut blandit semper. Proin ultrices "
    "luctus nulla eleifend pharetra. Vivamus ac feugiat quam. Vivamus "
    "venenatis auctor neque vel lacinia. Nulla lorem ipsum, ultrices sed odio "
    "vel, mattis aliquet odio. Nam suscipit in lacus eget volutpat. Cras lorem "
    "quam, interdum sit amet sem a, congue blandit urna. Lorem ipsum dolor sit "
    "amet, consectetur adipiscing elit. Quisque cursus iaculis felis ut "
    "faucibus. Pellentesque sed eleifend ipsum. Aenean eget neque eu diam "
    "lobortis sodales. Nam gravida nisl in lacus convallis, sed convallis ante "
    "rutrum. Nam facilisis massa leo, quis hendrerit nisi accumsan et. "
    "Praesent id orci nec lorem varius elementum eu eget purus. Nullam laoreet "
    "suscipit leo sit amet sodales. Mauris non nibh sit amet est faucibus "
    "ullamcorper. Maecenas imperdiet velit ut blandit semper. Proin ultrices "
    "luctus nulla eleifend pharetra. Vivamus ac feugiat quam. Vivamus "
    "venenatis auctor neque vel lacinia. Nulla lorem ipsum, ultrices sed odio "
    "vel, mattis aliquet odio. Nam suscipit in lacus eget volutpat. Cras lorem "
    "quam, interdum sit amet sem a, congue blandit urna.";

constexpr int READ_SIZE = 4096;

inline int test_setup_listener_get_pfd(int port, event_manager *ev) {
  int listener_fd, listener_pfd;
  int yes = 1;
  addrinfo hints, *server_info, *traverser;

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // tcp
  hints.ai_flags = AI_PASSIVE;     // use local IP

  int ret_addrinfo, ret_bind;
  ret_addrinfo =
      getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &server_info);

  for (traverser = server_info; traverser != NULL;
       traverser = traverser->ai_next) {

    listener_pfd = ev->socket_create_normally(
        traverser->ai_family, traverser->ai_socktype, traverser->ai_protocol);
    listener_fd = ev->get_pfd_data(listener_pfd).fd;
    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
    setsockopt(listener_fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
    int keep_idle =
        100; // The time (in seconds) the connection needs to remain idle before
             // TCP starts sending keepalive probes, if the socket option
             // SO_KEEPALIVE has been set on this socket.  This option should
             // not be used in code intended to be portable.
    setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle,
               sizeof(keep_idle));
    int keep_interval =
        100; // The time (in seconds) between individual keepalive probes. This
             // option should not be used in code intended to be portable.
    setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval,
               sizeof(keep_interval));
    int keep_count = 5; // The maximum number of keepalive probes TCP should
                        // send before dropping the connection.  This option
                        // should not be used in code intended to be portable.
    setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPCNT, &keep_count,
               sizeof(keep_count));
    ret_bind = bind(listener_fd, traverser->ai_addr, traverser->ai_addrlen);
  }

  if (ret_addrinfo < 0 || ret_bind < 0) {
    std::cerr << "addrinfo or bind failed: " << ret_addrinfo << " # "
              << ret_bind << "\n";
    return -1;
  }

  freeaddrinfo(server_info);

  if (listen(listener_fd, 10) < 0) {
    return -1;
  }

  return listener_pfd;
}

inline void test_accept_callback(event_manager *ev, int listener_fd,
                                 sockaddr_storage *user_data, socklen_t size,
                                 uint64_t pfd, int op_res_now) {
  if(op_res_now < 0) {
    std::cout << "accept errored with: " << op_res_now << "\n";
    // if errored and also not LIVING then shutdown the socket properly just in case
    if(ev->get_living_state() > event_manager::living_state::LIVING) {
      ev->shutdown_and_close_normally(pfd);
    }

    return;
  }

  std::cout << "Got a pfd (" << pfd << ")\n";

  ev->queue_accept(listener_fd); // want it to continue listening

  uint8_t *buff = new uint8_t[READ_SIZE];
  ev->queue_read(pfd, buff, READ_SIZE);

  ev->submit_all_queued_sqes();
}

inline void test_read_callback(event_manager *ev, processed_data read_metadata,
                               uint64_t pfd) {
  if (read_metadata.op_res_now < 0) {
    std::cout << "read callback got res " << read_metadata.op_res_now << "\n";
    if (read_metadata.op_res_now == -ECONNREFUSED) {
      std::cout << "\tconection refused...\n";
      // deal with this
    } else if (read_metadata.op_res_now == -ECANCELED) {
      std::cout << "\trequest cancelled...\n";
    }

    // if errored and also not LIVING then shutdown the socket properly just in case
    if(ev->get_living_state() > event_manager::living_state::LIVING) {
      ev->shutdown_and_close_normally(pfd);
    }

    // etc, deal with errors as you please
    return;
  }

  if (read_metadata.op_res_now == 0) {
    // end of file reached, i.e no more data can be read from it
    std::cout << ev->submit_shutdown(pfd, SHUT_WR) << " is shutdown code\n";
    // try to close writing side of the connection, since we can't send anything
    // anymore
    return;
  }

  size_t amount_read =
      read_metadata.amount_processed_before + read_metadata.op_res_now;

  std::cout << "Received message: \""
            << std::string(reinterpret_cast<char *>(read_metadata.buff),
                           amount_read)
            << "\", length was " << amount_read << ", with pfd: " << pfd
            << "\n";

  // these 5 requests aren't read in the main thread for one of the sockets, but they are still cancelled
  std::memset(read_metadata.buff, 0, read_metadata.length);
  ev->submit_read(pfd, read_metadata.buff, read_metadata.length);
  ev->submit_read(pfd, read_metadata.buff, read_metadata.length);
  ev->submit_read(pfd, read_metadata.buff, read_metadata.length);
  ev->submit_read(pfd, read_metadata.buff, read_metadata.length);
  ev->submit_read(pfd, read_metadata.buff, read_metadata.length);

  char *write_buff = new char[long_message.size()];
  std::memcpy(write_buff, text_message_2.c_str(), text_message_2.size());
  ev->submit_write(pfd, reinterpret_cast<uint8_t *>(write_buff),
                   text_message_2.size());
}

inline void test_write_callback(event_manager *ev,
                                processed_data write_metadata, uint64_t pfd) {
  if (write_metadata.op_res_now < 0) {
    std::cout << "write callback got res " << write_metadata.op_res_now << "\n";
    if (write_metadata.op_res_now == 111) {
      // deal with this
    }
    // etc, deal with errors as you please

    // if errored and also not LIVING then shutdown the socket properly just in case
    if(ev->get_living_state() > event_manager::living_state::LIVING) {
      ev->shutdown_and_close_normally(pfd);
    }

    return;
  }

  size_t amount_read =
      write_metadata.amount_processed_before + write_metadata.op_res_now;
  
  std::cout << "Wrote message: \""
            << std::string(reinterpret_cast<char *>(write_metadata.buff),
                           amount_read)
            << "\", length was " << amount_read << ", with pfd: " << pfd
            << "\n";
  free(write_metadata.buff);

  ev->submit_shutdown(pfd, SHUT_RD);
}

inline void test_write_callback_without_shutdown(event_manager *ev,
                                processed_data write_metadata, uint64_t pfd) {
  if (write_metadata.op_res_now < 0) {
    std::cout << "write callback got res " << write_metadata.op_res_now << "\n";
    if (write_metadata.op_res_now == 111) {
      // deal with this
    }
    // etc, deal with errors as you please

    // if errored and also not LIVING then shutdown the socket properly just in case
    if(ev->get_living_state() > event_manager::living_state::LIVING) {
      ev->shutdown_and_close_normally(pfd);
    }

    return;
  }

  size_t amount_read =
      write_metadata.amount_processed_before + write_metadata.op_res_now;
  
  std::cout << "Wrote message: \""
            << std::string(reinterpret_cast<char *>(write_metadata.buff),
                           amount_read)
            << "\", length was " << amount_read << ", with pfd: " << pfd
            << "\n";
  free(write_metadata.buff);
}

inline void test_shutdown_callback(event_manager *ev, int how, uint64_t pfd, int op_res_now) {
  if(op_res_now < 0) {
    std::cout << "shutdown errored with: " << op_res_now << "\n";
    // if errored and also not LIVING then shutdown the socket properly just in case
    if(ev->get_living_state() > event_manager::living_state::LIVING) {
      ev->shutdown_and_close_normally(pfd);
    }

    return;
  }

  switch (how) {
  case SHUT_RD: {
    std::cout << "shut down stage 1 done\n";
    break;
  }
  case SHUT_WR: {
    std::cout << "shut down stage 2 done\n";
    std::cout << ev->submit_close(pfd) << " is close code\n";
    ;
    break;
  };
  }
}

inline void test_close_callback(event_manager *ev, uint64_t pfd, int op_res_now) {
  if(op_res_now < 0) {
    std::cout << "close errored with: " << op_res_now << "\n";
    // if errored and also not LIVING then shutdown the socket properly just in case
    if(ev->get_living_state() > event_manager::living_state::LIVING) {
      ev->shutdown_and_close_normally(pfd);
    }

    return;
  }

  std::cout << "pfd closed: " << pfd << "\n";
}

inline void test_event_callback(event_manager *ev, uint64_t additional_info,
                                int pfd, int op_res_now) {
  if(op_res_now < 0) {
    std::cout << "read event errored with: " << op_res_now << "\n";
    // if errored and also not LIVING then shutdown the socket properly just in case
    if(ev->get_living_state() > event_manager::living_state::LIVING) {
      ev->shutdown_and_close_normally(pfd);
    }

    return;
  }

  std::cout << "received event signal for: " << pfd
            << ", with additional info: " << additional_info << "\n";
}

inline int connect_to_local_test_server(const char *port) {
  struct addrinfo hints, *res;
  int sockfd;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo("127.0.0.1", port, &hints, &res) < 0) {
    return -1;
  }

  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
    return -2;
  }

  return sockfd;
}

#endif