#ifndef TEST_FUNCTIONS
#define TEST_FUNCTIONS

#include <iostream>
#include <thread>
#include <cstring>

#include <netdb.h>
#include <netinet/tcp.h>
#include <asm-generic/errno.h>

#include "../event_manager.hpp"

const std::string text_message = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque cursus "
                                 "iaculis felis ut faucibus. Pellentesque sed eleifend ipsum. Aenean eget "
                                 "neque eu diam lobortis sodales. Nam gravida nisl in lacus convallis.";
const std::string text_message_2 =
    "Cras lorem quam, interdum sit amet sem a, congue blandit urna. Lorem "
    "ipsum dolor sit amet, consectetur adipiscing elit. Quisque cursus iaculis "
    "felis ut faucibus. Pellentesque sed eleifend ipsum. Vivamus ac feugiat "
    "quam. Vivamus venenatis auctor neque vel lacinia. Nulla lorem ipsum, "
    "ultrices sed odio vel, mattis aliquet odio. Nam suscipit in lacus eget "
    "volutpat. Cras lorem quam, interdum sit amet sem a, congue blandit urna.";
const std::string long_message = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque cursus "
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
  int listener_fd{};
  int yes = 1;
  addrinfo hints, *server_info, *traverser;

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // tcp
  hints.ai_flags = AI_PASSIVE;     // use local IP

  int ret_addrinfo, ret_bind;
  ret_addrinfo = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &server_info);

  for (traverser = server_info; traverser != NULL; traverser = traverser->ai_next) {

    listener_fd = socket(traverser->ai_family, traverser->ai_socktype, traverser->ai_protocol);
    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
    setsockopt(listener_fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
    int keep_idle = 100; // The time (in seconds) the connection needs to remain idle before
                         // TCP starts sending keepalive probes, if the socket option
                         // SO_KEEPALIVE has been set on this socket.  This option should
                         // not be used in code intended to be portable.
    setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle));
    int keep_interval = 100; // The time (in seconds) between individual keepalive probes. This
                             // option should not be used in code intended to be portable.
    setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval));
    int keep_count = 5; // The maximum number of keepalive probes TCP should
                        // send before dropping the connection.  This option
                        // should not be used in code intended to be portable.
    setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPCNT, &keep_count, sizeof(keep_count));
    ret_bind = bind(listener_fd, traverser->ai_addr, traverser->ai_addrlen);
  }

  if (ret_addrinfo < 0 || ret_bind < 0) {
    std::cerr << "addrinfo or bind failed: " << ret_addrinfo << " # " << ret_bind << "\n";
    return -1;
  }

  freeaddrinfo(server_info);

  if (listen(listener_fd, 10) < 0) {
    return -1;
  }

  return ev->pass_fd_to_event_manager(listener_fd, true);
}

inline int connect_to_local_test_server(const char *port) {
  struct addrinfo hints, *res;
  int sockfd;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo("127.0.0.1", port, &hints, &res) < 0) {
    freeaddrinfo(res);
    return -1;
  }

  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
    freeaddrinfo(res);
    return -2;
  }

  freeaddrinfo(res);
  return sockfd;
}

struct ref_count_buff {
  int uses{};
  uint8_t *buff{};
};

class test_server : public server_methods {
public:
  bool write_callback_no_shutdown = false;

  int transaction_id = 0;
  std::vector<ref_count_buff> ref_counted_data{};

  void ref_counted_data_delete(int id) {
    if((size_t)id < ref_counted_data.size()) {
      --ref_counted_data[id].uses;
      if(ref_counted_data[id].uses == 0) {
        delete[] ref_counted_data[id].buff;
        ref_counted_data[id] = {};
      }
    }
  }

  int ref_counted_data_insert(size_t buff_size, int uses) {
    int id = transaction_id++;
    uint8_t *buff = new uint8_t[buff_size];
    if(ref_counted_data.size() <= (size_t)id) {
      ref_counted_data.resize(id+1);
    }
    ref_counted_data[id].buff = buff;
    ref_counted_data[id].uses = uses;

    return id;
  }

  void ref_counted_data_update_uses(int id, int new_uses) {
    if((size_t)id < ref_counted_data.size()) {
      ref_counted_data[id].uses = new_uses;
    }
  }

  uint8_t *ref_counted_data_get_buffer(int id) {
    if((size_t)id < ref_counted_data.size()) {
      return ref_counted_data[id].buff;
    }
    return nullptr;
  }

  void accept_callback(int listener_pfd, sockaddr_storage *user_data, socklen_t size, uint64_t pfd,
                       int op_res_num, uint64_t additional_info) override {
    if (op_res_num < 0) {
      std::cout << "accept errored with: " << op_res_num << "\n";
      // if errored and also not LIVING then shutdown the socket properly just
      // in case
      if (ev->get_living_state() > event_manager::living_state::LIVING) {
        ev->close_pfd(pfd, additional_info);
      }
      return;
    }

    std::cout << "Got a pfd (" << pfd << ")\n";

    ev->queue_accept(listener_pfd); // want it to continue listening

    // id for this transaction
    int id = ref_counted_data_insert(READ_SIZE, 1);
    ev->queue_read(pfd, ref_counted_data_get_buffer(id), READ_SIZE, id);

    ev->submit_all_queued_sqes();
  }

  void read_callback(processed_data read_metadata, uint64_t pfd, uint64_t additional_info) override {
    int id = additional_info;
    if (read_metadata.op_res_num < 0) {
      std::cout << "read callback got res " << read_metadata.op_res_num << "\n";
      if (read_metadata.op_res_num == -ECONNREFUSED) {
        std::cout << "\tconection refused...\n";
        // deal with this
      } else if (read_metadata.op_res_num == -ECANCELED) {
        std::cout << "\trequest cancelled...\n";
      }

      // if errored and also not LIVING then shutdown the socket properly just
      // in case
      if (ev->get_living_state() > event_manager::living_state::LIVING) {
        ev->close_pfd(pfd, additional_info);
      }

      // clean up allocated data
      ref_counted_data_delete(id);

      // etc, deal with errors as you please
      return;
    }

    if (read_metadata.op_res_num == 0) {
      // end of file reached, i.e no more data can be read from it
      ev->close_pfd(pfd, additional_info);
      // try to close writing side of the connection, since we can't send
      // anything anymore

      // clean up allocated data
      ref_counted_data_delete(id);
      return;
    }

    size_t amount_read = read_metadata.op_res_num;

    std::cout << "Received message: \""
              << std::string(reinterpret_cast<char *>(read_metadata.buff), amount_read) << "\", length was "
              << amount_read << ", with pfd: " << pfd << "\n";

    // these 5 requests aren't read in the main thread for one of the sockets,
    // but they are still cancelled
    std::memset(read_metadata.buff, 0, read_metadata.length);

    ref_counted_data_update_uses(id, 5);

    ev->submit_read(pfd, read_metadata.buff, read_metadata.length, id);
    ev->submit_read(pfd, read_metadata.buff, read_metadata.length, id);
    ev->submit_read(pfd, read_metadata.buff, read_metadata.length, id);
    ev->submit_read(pfd, read_metadata.buff, read_metadata.length, id);
    ev->submit_read(pfd, read_metadata.buff, read_metadata.length, id);

    char *write_buff = new char[long_message.size()];
    std::memcpy(write_buff, text_message_2.c_str(), text_message_2.size());
    ev->submit_write(pfd, reinterpret_cast<uint8_t *>(write_buff), text_message_2.size());
  }

  void write_callback(processed_data write_metadata, uint64_t pfd, uint64_t additional_info) override {
    if (write_callback_no_shutdown) {
      if (write_metadata.op_res_num < 0) {
        std::cout << "write callback got res " << write_metadata.op_res_num << "\n";
        if (write_metadata.op_res_num == 111) {
          // deal with this
        }
        // etc, deal with errors as you please

        // if errored and also not LIVING then shutdown the socket properly just
        // in case
        if (ev->get_living_state() > event_manager::living_state::LIVING) {
          ev->close_pfd(pfd, additional_info);
        }

        return;
      }

      size_t amount_read = write_metadata.op_res_num;

      std::cout << "Wrote message: \""
                << std::string(reinterpret_cast<char *>(write_metadata.buff), amount_read)
                << "\", length was " << amount_read << ", with pfd: " << pfd << "\n";
      delete[] write_metadata.buff;
    } else {
      if (write_metadata.op_res_num < 0) {
        std::cout << "write callback got res " << write_metadata.op_res_num << "\n";
        if (write_metadata.op_res_num == 111) {
          // deal with this
        }
        // etc, deal with errors as you please

        // if errored and also not LIVING then shutdown the socket properly just
        // in case
        if (ev->get_living_state() > event_manager::living_state::LIVING) {
          ev->close_pfd(pfd, additional_info);
        }

        return;
      }

      size_t amount_read = write_metadata.op_res_num;

      std::cout << "Wrote message: \""
                << std::string(reinterpret_cast<char *>(write_metadata.buff), amount_read)
                << "\", length was " << amount_read << ", with pfd: " << pfd << "\n";
      delete[] write_metadata.buff;

      ev->close_pfd(pfd, additional_info);
    }
  }

  void event_callback(int pfd, int op_res_num, uint64_t additional_info) override {
    if (op_res_num < 0) {
      std::cout << "read event errored with: " << op_res_num << "\n";
      // if errored and also not LIVING then shutdown the socket properly just
      // in case
      if (ev->get_living_state() > event_manager::living_state::LIVING) {
        ev->close_pfd(pfd, additional_info);
      }

      return;
    }

    std::cout << "received event signal for: " << pfd << ", with additional info: " << additional_info
              << "\n";
  }

  void close_callback(uint64_t pfd, int op_res_num, uint64_t additional_info) override {
    if (op_res_num < 0) {
      std::cout << "close errored with: " << op_res_num << "\n";
      // if errored and also not LIVING then shutdown the socket properly just
      // in case
      if (ev->get_living_state() > event_manager::living_state::LIVING) {
        ev->close_pfd(pfd, additional_info);
      }

      return;
    }

    std::cout << "pfd closed: " << pfd << "\n";
  }
};

class test_server_with_killed_callback : public test_server {
public:
  void killed_callback() override { std::cout << "\n\n\tthe server has been killed\n\n"; }
};

#endif