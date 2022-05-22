#include "event_manager.hpp"

#include <cstdint>
#include <cstring>
#include <thread>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "vendor/doctest/doctest/doctest.h"

#include <netinet/tcp.h>
#include <netdb.h>

int test_setup_listener(int port) {
  int listener_fd;
  int yes = 1;
  addrinfo hints, *server_info, *traverser;

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       //IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; //tcp
  hints.ai_flags = AI_PASSIVE;     //use local IP

  int ret_addrinfo, ret_bind;
  ret_addrinfo = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &server_info);

  for (traverser = server_info; traverser != NULL; traverser = traverser->ai_next) {
    listener_fd = socket(traverser->ai_family, traverser->ai_socktype, traverser->ai_protocol);
    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    setsockopt(listener_fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
    setsockopt(listener_fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
    int keep_idle = 100; // The time (in seconds) the connection needs to remain idle before TCP starts sending keepalive probes, if the socket option SO_KEEPALIVE has been set on this socket.  This option should not be used in code intended to be portable.
    setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle));
    int keep_interval = 100; // The time (in seconds) between individual keepalive probes. This option should not be used in code intended to be portable.
    setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval));
    int keep_count = 5; // The maximum number of keepalive probes TCP should send before dropping the connection.  This option should not be used in code intended to be portable.
    setsockopt(listener_fd, IPPROTO_TCP, TCP_KEEPCNT, &keep_count, sizeof(keep_count));
    ret_bind = bind(listener_fd, traverser->ai_addr, traverser->ai_addrlen);
  }

  if(ret_addrinfo < 0 || ret_bind < 0) {
    std::cerr << "addrinfo or bind failed: " << ret_addrinfo << " # " << ret_bind << "\n";
    return -1;
  }

  freeaddrinfo(server_info);
  
  if(listen(listener_fd, 10) < 0) {
    return -1;
  }

  return listener_fd;
}

void test_accept_callback(event_manager *ev, int listener_fd, sockaddr_storage *user_data, socklen_t size, uint64_t pfd) {
  std::cout << "got a pfd: " << pfd << "\n";
  std::cout << "the fd is " << pfd_data(pfd).fd << "\n";

  ev->queue_accept(listener_fd); // want it to continue listening

  constexpr int read_size = 4096;
  uint8_t *buff = new uint8_t[read_size];
  ev->queue_read(pfd, buff, read_size);

  ev->submit_all_queued_sqes();
}

int connect_to_local_test_server() {
  struct addrinfo hints, *res;
  int sockfd;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if(getaddrinfo("127.0.0.1", "4000", &hints, &res) < 0){
    return -1;
  }

  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  if(connect(sockfd, res->ai_addr, res->ai_addrlen) < 0){
    return -2;
  }

  return sockfd;
}

TEST_CASE("event manager full tests") {
  // tested functions:
  //     submit_write, open_normally_get_pfd, await_single_message, fstat_normally, stat_normally, submit_read, unlink_normally,
  //     all of the pfd_data methods implicitly, submit_all_queued_sqes, queue_read, queue_write, close_pfd (but not submit_close/queue_close)
  // as a result of this, start() should work since it is just await_single_message in a loop
  SUBCASE("writing a new file, stat/fstat contents, read contents and check that too") {
    event_manager ev{};

    std::string filename = "test.txt";
    std::string data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi et ipsum pellentesque, vestibulum dolor sed, egestas nibh.\n";

    auto new_file_pfd = ev.open_normally_get_pfd(filename.c_str(), O_WRONLY | O_CREAT, 0666);
    REQUIRE(ev.submit_write(new_file_pfd, reinterpret_cast<uint8_t*>(data.data()), data.length()) == 1);
    // == 1 above since should have submitted 1 sqe
    ev.await_single_message();

    struct stat statbuf{};
    REQUIRE(ev.fstat_normally(new_file_pfd, &statbuf) == 0);
    REQUIRE(statbuf.st_size == data.length());
    ev.close_pfd(new_file_pfd);

    std::memset(&statbuf, 0, sizeof(statbuf));
    REQUIRE(ev.stat_normally(filename.c_str(), &statbuf) == 0);
    REQUIRE(statbuf.st_size == data.length());

    auto that_file_pfd = ev.open_normally_get_pfd(filename.c_str(), O_RDONLY);
    char buff[1024];
    REQUIRE(ev.submit_read(that_file_pfd, reinterpret_cast<uint8_t*>(&buff[0]), sizeof(buff)) == 1);
    // == 1 above since should have submitted 1 sqe
    ev.await_single_message();
    REQUIRE(ev.close_pfd(that_file_pfd) == 0);

    REQUIRE(ev.unlink_normally(filename.c_str()) == 0);

    REQUIRE(strcmp(data.c_str(), buff) == 0);
  }

  SUBCASE("network operations") {
    event_manager ev{};

    event_manager_callbacks ev_cbs{};
    ev_cbs.accept_cb = test_accept_callback;
    ev.set_callbacks(ev_cbs);
    
    std::thread t([&] () {
      ev.start();
    });

    int tmp_listener = test_setup_listener(4000);
    REQUIRE(tmp_listener >= 0);
    REQUIRE(ev.submit_accept(tmp_listener) == 1);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    int sockfd = connect_to_local_test_server();
    REQUIRE(sockfd >= 0);

    

    t.join();
  }
}