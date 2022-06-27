#include "test_functions.hpp"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../vendor/doctest/doctest/doctest.h"

TEST_CASE("event manager full tests") {
  // tested functions:
  //     submit_write, open_normally_get_pfd, await_single_message,
  //     fstat_normally, stat_normally, submit_read, unlink_normally, all of the
  //     pfd_data methods implicitly, submit_all_queued_sqes, queue_read,
  //     queue_write, close_pfd (but not submit_close/queue_close)
  // as a result of this, start() should work since it is just
  // await_single_message in a loop
  SUBCASE("writing a new file, stat/fstat contents, read contents and check "
          "that too") {
    event_manager ev{};

    std::thread t([&]() { ev.start(); });

    std::string filename = "test.txt";
    std::string data =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi et "
        "ipsum pellentesque, vestibulum dolor sed, egestas nibh.\n";

    auto new_file_pfd =
        ev.open_normally_get_pfd(filename.c_str(), O_WRONLY | O_CREAT, 0666);
    REQUIRE(ev.submit_write(new_file_pfd,
                            reinterpret_cast<uint8_t *>(data.data()),
                            data.length()) == 1);
    // == 1 above since should have submitted 1 sqe

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    struct stat statbuf {};
    REQUIRE(ev.fstat_normally(new_file_pfd, &statbuf) == 0);
    REQUIRE(statbuf.st_size == data.length());
    ev.close_pfd(new_file_pfd);

    std::memset(&statbuf, 0, sizeof(statbuf));
    REQUIRE(ev.stat_normally(filename.c_str(), &statbuf) == 0);
    REQUIRE(statbuf.st_size == data.length());

    auto that_file_pfd = ev.open_normally_get_pfd(filename.c_str(), O_RDONLY);
    char buff[1024];
    REQUIRE(ev.submit_read(that_file_pfd, reinterpret_cast<uint8_t *>(&buff[0]),
                           sizeof(buff)) == 1);
    // == 1 above since should have submitted 1 sqe

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(ev.close_pfd(that_file_pfd) == 0);

    REQUIRE(ev.unlink_normally(filename.c_str()) == 0);

    REQUIRE(strcmp(data.c_str(), buff) == 0);

    ev.kill();
    t.join();
  }

  SUBCASE("network operations") {
    event_manager ev{};

    event_manager_callbacks ev_cbs{};
    ev_cbs.accept_cb = test_accept_callback;
    ev_cbs.read_cb = test_read_callback;
    ev_cbs.write_cb = test_write_callback;
    ev_cbs.shutdown_cb = test_shutdown_callback;
    ev_cbs.close_cb = test_close_callback;
    ev_cbs.event_cb = test_event_callback;
    ev.set_callbacks(ev_cbs);

    std::thread t([&]() { ev.start(); });

    int tmp_listener = test_setup_listener(4000);
    REQUIRE(tmp_listener >= 0);
    REQUIRE(ev.submit_accept(tmp_listener) == 1);

    int sockfd = connect_to_local_test_server();
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

    ev.kill(); // kill the server to exit

    t.join();
  }

  SUBCASE("event callbacks") {
    event_manager ev{};

    event_manager_callbacks ev_cbs{};
    ev_cbs.event_cb = test_event_callback;
    ev.set_callbacks(ev_cbs);

    std::thread t([&]() { ev.start(); });

    const int CUSTOM_EVENT_ID = 122343;
    auto pfd = ev.create_event_fd_normally();
    ev.submit_generic_event(pfd, CUSTOM_EVENT_ID);

    REQUIRE(ev.event_alert_normal(pfd) == 0);

    ev.kill();

    t.join();
  }
}