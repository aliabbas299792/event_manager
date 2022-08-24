#include "test_functions.hpp"
#include <chrono>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../vendor/doctest/doctest/doctest.h"

TEST_CASE("event manager full tests") {
  // tested functions:
  //     submit_write, open_get_pfd_normally, await_single_message,
  //     fstat_normally, stat_normally, submit_read, unlink_normally, all of the
  //     pfd_data methods implicitly, submit_all_queued_sqes, queue_read,
  //     queue_write, close_pfd (but not submit_close/queue_close)
  // as a result of this, start() should work since it is just
  // await_single_message in a loop
  // some stuff which is harder to test without adding debug code into the
  // library is just printed to the console

  SUBCASE("writing a new file, stat/fstat contents, read contents and check "
          "that too") {
    std::cout << "\033[1;31mwriting a new file, stat/fstat contents, read "
                 "contents and check that too\033[0m\n";

    server_methods sm{}; // no callbacks set
    event_manager ev{};
    ev.set_server_methods(&sm);

    std::thread t([&]() { ev.start(); });

    std::string filename = "test.txt";
    std::string data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi et "
                       "ipsum pellentesque, vestibulum dolor sed, egestas nibh.\n";

    auto new_file_pfd = ev.open_get_pfd_normally(filename.c_str(), O_WRONLY | O_CREAT, 0666);
    REQUIRE(ev.submit_write(new_file_pfd, reinterpret_cast<uint8_t *>(data.data()), data.length()) == 1);
    // == 1 above since should have submitted 1 sqe

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    struct stat statbuf {};
    REQUIRE(ev.fstat_normally(new_file_pfd, &statbuf) == 0);
    REQUIRE(statbuf.st_size == data.length());
    ev.close_pfd(new_file_pfd);

    std::memset(&statbuf, 0, sizeof(statbuf));
    REQUIRE(ev.stat_normally(filename.c_str(), &statbuf) == 0);
    REQUIRE(statbuf.st_size == data.length());

    auto that_file_pfd = ev.open_get_pfd_normally(filename.c_str(), O_RDONLY);
    char buff[1024];
    REQUIRE(ev.submit_read(that_file_pfd, reinterpret_cast<uint8_t *>(&buff[0]), sizeof(buff)) == 1);
    // == 1 above since should have submitted 1 sqe

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(ev.close_pfd(that_file_pfd) == 0);

    REQUIRE(ev.unlink_normally(filename.c_str()) == 0);

    REQUIRE(strcmp(data.c_str(), buff) == 0);

    ev.kill();
    t.join();
    std::cout << "\033[1;32mwriting a new file, stat/fstat contents, read "
                 "contents and check that too finished\033[0m\n\n\n";
  }

  SUBCASE("network operations") {
    std::cout << "\033[1;31mnetwork operations\033[0m\n";

    test_server t{};
    event_manager ev{&t};

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

  SUBCASE("network operations but don't shutdown the socket") {
    std::cout << "\033[1;31mnetwork operations but don't shutdown the "
                 "socket\033[0m\n";

    test_server t{};
    t.write_callback_no_shutdown = true;
    event_manager ev{&t};

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

  SUBCASE("event callbacks") {
    std::cout << "\033[1;31mevent callbacks\033[0m\n";

    test_server t{};
    event_manager ev{&t};

    std::thread t1([&]() { ev.start(); });

    const int CUSTOM_EVENT_ID = 122343;
    auto pfd = ev.create_event_fd_normally();
    ev.submit_generic_event(pfd, CUSTOM_EVENT_ID);

    REQUIRE(ev.event_alert_normally(pfd) == 0);

    ev.kill();

    t1.join();
    std::cout << "\033[1;32mevent callbacks finished\033[0m\n\n\n";
  }
}