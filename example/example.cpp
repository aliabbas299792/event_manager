#include <thread>
#include <iostream>

#include <cstring>
#include <fcntl.h>

#include "../event_manager.hpp"

class test_server : public server_methods {
public:
  void write_callback(processed_data write_metadata, uint64_t pfd, uint64_t additional_info) override {
    if(write_metadata.op_res_num < 0) {
      ev->close_pfd(pfd);
      return;
    }
    
    size_t length = write_metadata.op_res_num;
    std::cout << "Wrote " << length << " bytes of data, the data was: "
              << std::string(reinterpret_cast<char *>(write_metadata.buff), length) << "\n";
  }

  void read_callback(processed_data read_metadata, uint64_t pfd, uint64_t additional_info) override {
    if (read_metadata.op_res_num > 0) {
      size_t length = read_metadata.op_res_num;

      std::cout << "Read " << length << " bytes of data, the data was: "
                << std::string(reinterpret_cast<char *>(read_metadata.buff), length) << "\n";
    }
  }
};

int main() {
  test_server t{};
  event_manager ev{&t};
  t.set_event_manager(&ev);

  std::thread t1([&]() { ev.start(); });

  std::string filename = "test.txt";
  std::string data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi et ipsum "
                     "pellentesque, vestibulum dolor sed, egestas nibh.\n";

  char write_buff[data.size()];
  data.copy(write_buff, data.size());

  auto new_file_fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0666);
  auto new_file_pfd = ev.pass_fd_to_event_manager(new_file_fd, false);

  ev.submit_write(new_file_pfd, reinterpret_cast<uint8_t *>(write_buff), data.length());
  // == 1 above since should have submitted 1 sqe

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  struct stat statbuf {};
  fstat(new_file_fd, &statbuf);
  ev.close_pfd(new_file_pfd);

  std::memset(&statbuf, 0, sizeof(statbuf));
  stat(filename.c_str(), &statbuf);

  auto that_file_fd = open(filename.c_str(), O_RDONLY);
  auto that_file_pfd = ev.pass_fd_to_event_manager(that_file_fd, false);
  char buff[1024];

  ev.submit_read(that_file_pfd, reinterpret_cast<uint8_t *>(&buff[0]), sizeof(buff));
  // == 1 above since should have submitted 1 sqe

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ev.close_pfd(that_file_pfd);

  unlink(filename.c_str()); // deletes the file

  ev.kill();
  t1.join();
}