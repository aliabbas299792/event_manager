#include "test_functions.hpp"
#include <bits/types/struct_iovec.h>
#include <chrono>
#include <fcntl.h>
#include <stdexcept>
#include <thread>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../vendor/doctest/doctest/doctest.h"

// tested functions:
//     submit_write, open_get_pfd_normally, await_single_message,
//     fstat_normally, stat_normally, submit_read, unlink_normally, all of the
//     pfd_data methods implicitly, submit_all_queued_sqes, queue_read,
//     queue_write, close_pfd (but not submit_close/queue_close)
// as a result of this, start() should work since it is just
// await_single_message in a loop
// some stuff which is harder to test without adding debug code into the
// library is just printed to the console

TEST_CASE("event manager no callbacks") {
  std::cout << "\033[1;31mchecking that starting with no callbacks throws an exception\033[0m\n";
                
  event_manager ev{};
  REQUIRE_THROWS_AS(ev.start(), std::runtime_error);
  ev.kill();
  
  std::cout << "\033[1;32mchecking that starting with no callbacks throws an exception finished\033[0m\n\n\n";
}

TEST_CASE("writing a new file, stat/fstat contents, read contents and check "
        "that too") {
  std::cout << "\033[1;31mwriting a new file, stat/fstat contents, read "
                "contents and check that too\033[0m\n";

  server_methods sm{}; // no callbacks set
  event_manager ev{};
  ev.set_callbacks(&sm);

  sm.set_event_manager(&ev);

  std::thread t([&]() { ev.start(); });

  std::string filename = "test.txt";
  std::string data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi et "
                      "ipsum pellentesque, vestibulum dolor sed, egestas nibh.\n";

  auto new_file_fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0666);
  auto new_file_pfd = ev.pass_fd_to_event_manager(new_file_fd, false);
  REQUIRE(ev.submit_write(new_file_pfd, reinterpret_cast<uint8_t *>(data.data()), data.length()) == 1);
  // == 1 above since should have submitted 1 sqe

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  struct stat statbuf {};
  REQUIRE(fstat(new_file_fd, &statbuf) == 0);
  REQUIRE(statbuf.st_size == data.length());
  ev.close_pfd(new_file_pfd);

  std::memset(&statbuf, 0, sizeof(statbuf));
  REQUIRE(stat(filename.c_str(), &statbuf) == 0);
  REQUIRE(statbuf.st_size == data.length());

  auto that_file_fd = open(filename.c_str(), O_RDONLY);
  auto that_file_pfd = ev.pass_fd_to_event_manager(that_file_fd, false);
  char buff[1024] {};
  REQUIRE(ev.submit_read(that_file_pfd, reinterpret_cast<uint8_t *>(&buff[0]), sizeof(buff)) == 1);
  // == 1 above since should have submitted 1 sqe

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  REQUIRE(ev.close_pfd(that_file_pfd) == 0);

  REQUIRE(unlink(filename.c_str()) == 0);

  REQUIRE(strcmp(data.c_str(), buff) == 0);

  ev.kill();
  t.join();
  std::cout << "\033[1;32mwriting a new file, stat/fstat contents, read "
                "contents and check that too finished\033[0m\n\n\n";
}

TEST_CASE("network operations") {
  std::cout << "\033[1;31mnetwork operations\033[0m\n";

  test_server t{};
  event_manager ev{&t};

  t.set_event_manager(&ev);

  std::thread t1([&]() { ev.start(); });

  int tmp_listener = test_setup_listener_get_pfd(4000, &ev);
  REQUIRE(tmp_listener >= 0);
  REQUIRE(ev.submit_accept(tmp_listener) == 1);

  std::cout << tmp_listener << " ## \n";

  int sockfd = connect_to_local_test_server("4000");
  REQUIRE(sockfd >= 0);

  REQUIRE(write(sockfd, text_message.c_str(), text_message.size()) >= 0);

  char read_buff[READ_SIZE];
  std::memset(read_buff, 0, sizeof(read_buff));
  read(sockfd, read_buff, sizeof(read_buff));

  REQUIRE(strcmp(read_buff, text_message_2.c_str()) == 0);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // the test write callback closes the socket, so read should return 0 (i.e
  // end of file)
  REQUIRE(read(sockfd, read_buff, 1) == 0);

  std::this_thread::sleep_for(std::chrono::seconds(1));

  ev.kill(); // kill the server to exit

  t1.join();
  std::cout << "\033[1;32mnetwork operations finished\033[0m\n\n\n";
}

TEST_CASE("network operations but don't shutdown the socket") {
  std::cout << "\033[1;31mnetwork operations but don't shutdown the "
                "socket\033[0m\n";

  test_server_with_killed_callback t{};
  t.write_callback_no_shutdown = true;
  event_manager ev{&t};

  t.set_event_manager(&ev);

  std::thread t1([&]() { ev.start(); });

  int tmp_listener = test_setup_listener_get_pfd(4001, &ev);
  REQUIRE(tmp_listener >= 0);
  REQUIRE(ev.submit_accept(tmp_listener) == 1);

  std::cout << tmp_listener << " ## \n";

  int sockfd = connect_to_local_test_server("4001");
  REQUIRE(sockfd >= 0);

  if (sockfd >= 0) {

    REQUIRE(write(sockfd, text_message.c_str(), text_message.size()) >= 0);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    ev.kill(); // kill the server to exit

    t1.join();
  }
  std::cout << "\033[1;32mnetwork operations but don't shutdown the socket "
                "finished\033[0m\n\n\n";
}

TEST_CASE("event callbacks") {
  std::cout << "\033[1;31mevent callbacks\033[0m\n";

  test_server t{};
  event_manager ev{&t};
  t.set_event_manager(&ev);

  std::thread t1([&]() { ev.start(); });

  const int CUSTOM_EVENT_ID = 122343;
  auto pfd = ev.create_event_fd_normally();
  ev.submit_generic_event(pfd, CUSTOM_EVENT_ID);

  REQUIRE(ev.event_alert_normally(pfd) == 0);

  ev.kill();

  t1.join();
  std::cout << "\033[1;32mevent callbacks finished\033[0m\n\n\n";
}

TEST_CASE("readv/writev") {
  std::cout << "\033[1;31mreadv/writev callbacks\033[0m\n";

  test_server t{};
  event_manager ev{&t};
  t.set_event_manager(&ev);

  std::thread t1([&]() { ev.start(); });

  std::string filename = "test.txt";
  std::string data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi et "
                      "ipsum pellentesque, vestibulum dolor sed, egestas nibh.\n";

  struct iovec iovs[1024];
  for (int i = 0; i < 1024; i++) {
    iovs[i].iov_base = new char[data.size()+1]{};
    std::memcpy(iovs[i].iov_base, data.c_str(), data.size());
    iovs[i].iov_len = data.size();
  }

  auto new_file_fd = open(filename.c_str(), O_RDWR | O_CREAT, 0666);
  auto new_file_pfd = ev.pass_fd_to_event_manager(new_file_fd, false);

  std::cout << ev.submit_writev(new_file_pfd, iovs, 1024) << " is writev response code\n";

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  struct iovec iovs2[1024];
  for (int i = 0; i < 1024; i++) {
    iovs2[i].iov_base = new char[data.size()+1]{};
    iovs2[i].iov_len = data.size();
  }

  std::cout << ev.submit_readv(new_file_pfd, iovs2, 1024) << " is readv response code\n";

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (int i = 0; i < 1024; i++) {
    // require what was written to be identical to what was read
    REQUIRE(strcmp((const char *)iovs[i].iov_base, (const char *)iovs2[i].iov_base) == 0);
    delete[](char *) iovs[i].iov_base;
    delete[](char *) iovs2[i].iov_base;
  }

  REQUIRE(unlink(filename.c_str()) == 0);

  ev.kill();
  t1.join();

  std::cout << "\033[1;32mreadv/writev callbacks\033[0m\n";
}