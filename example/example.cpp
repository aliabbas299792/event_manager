#include "../event_manager.hpp"

#include <cstring>
#include <thread>

void example_write_callback(event_manager *ev, processed_data write_metadata,
                            uint64_t pfd) {
  auto length =
      write_metadata.amount_processed_before + write_metadata.op_res_now;
  std::cout << "Wrote " << length << " bytes of data, the data was: "
            << std::string(reinterpret_cast<char *>(write_metadata.buff),
                           length)
            << "\n";
}

void example_read_callback(event_manager *ev, processed_data read_metadata,
                           uint64_t pfd) {
  if (read_metadata.op_res_now > 0) {
    auto length =
        read_metadata.amount_processed_before + read_metadata.op_res_now;

    std::cout << "Read " << length << " bytes of data, the data was: "
              << std::string(reinterpret_cast<char *>(read_metadata.buff),
                             length)
              << "\n";
  }
}

int main() {
  event_manager ev{};

  // setting the callbacks
  event_manager_callbacks cbs{};
  cbs.write_cb = example_write_callback;
  cbs.read_cb = example_read_callback;

  ev.set_callbacks(cbs);

  std::thread t([&]() { ev.start(); });

  std::string filename = "test.txt";
  std::string data =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi et ipsum "
      "pellentesque, vestibulum dolor sed, egestas nibh.\n";

  char write_buff[data.size()];
  data.copy(write_buff, data.size());

  auto new_file_pfd =
      ev.open_normally_get_pfd(filename.c_str(), O_WRONLY | O_CREAT, 0666);

  ev.submit_write(new_file_pfd, reinterpret_cast<uint8_t *>(write_buff),
                  data.length());
  // == 1 above since should have submitted 1 sqe

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  struct stat statbuf {};
  ev.fstat_normally(new_file_pfd, &statbuf);
  ev.close_pfd(new_file_pfd);

  std::memset(&statbuf, 0, sizeof(statbuf));
  ev.stat_normally(filename.c_str(), &statbuf);

  auto that_file_pfd = ev.open_normally_get_pfd(filename.c_str(), O_RDONLY);
  char buff[1024];

  ev.submit_read(that_file_pfd, reinterpret_cast<uint8_t *>(&buff[0]),
                 sizeof(buff));
  // == 1 above since should have submitted 1 sqe

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ev.close_pfd(that_file_pfd);

  ev.unlink_normally(filename.c_str()); // deletes the file

  ev.kill();
  t.join();
}