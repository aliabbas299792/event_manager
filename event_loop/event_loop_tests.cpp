#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "vendor/doctest/doctest.h"

#include "communication/communication_channel.hpp"
#include "communication/communication_types.hpp"
#include "coroutine/io_awaitables.hpp"
#include "coroutine/task.hpp"
#include "event_loop/event_manager.hpp"
#include <coroutine>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <variant>

const std::string lorem_ipsum =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean "
    "ultricies\n"
    "ex sit amet orci tincidunt, a viverra sem suscipit. Phasellus non quam\n"
    "bibendum, molestie enim vel, placerat nibh. Mauris elementum, nisi et\n"
    "fringilla auctor, velit justo porttitor neque, ut viverra neque magna\n"
    "quis urna. Mauris eu libero lacinia, pulvinar purus nec, dapibus mi.\n"
    "Aliquam id accumsan nunc, vitae molestie diam. Proin eget aliquet orci,\n"
    "vitae pretium ante. Proin libero augue, vulputate eget iaculis vel,\n"
    "ultricies at velit. Phasellus euismod nisl vitae lacus scelerisque\n"
    "imperdiet sit amet vitae mi. Quisque rutrum leo et placerat iaculis.\n"
    "Fusce ac mattis eros, aliquet fringilla tellus. Quisque mauris mauris,\n"
    "dapibus nec orci et, lobortis tincidunt mauris.";

const std::string the_grand_inquisitor =
    "Upon my word, man is created weaker and more base than you supposed! Can\n"
    "he, can he perform the deeds of which you are capable? In respecting him\n"
    "so much you acted as though you had ceased to have compassion for him,\n"
    "because you demanded too much of him—and yet who was this? The very one\n"
    "you had loved more than yourself! Had you respected him less you would\n"
    "have demanded of him less, and that would have been closer to love, for\n"
    "his burden would have been lighter. He is weak and dishonourable.";

const std::string expected_output = R"((*) Read this data:
> Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean ultricies
> ex sit amet orci tincidunt, a viverra sem suscipit. Phasellus non quam
> bibendum, molestie enim vel, placerat nibh. Mauris elementum, nisi et
> fringilla auctor, velit justo porttitor neque, ut viverra neque magna
> quis urna. Mauris eu libero lacinia, pulvinar purus nec, dapibus mi.
> Aliquam id accumsan nunc, vitae molestie diam. Proin eget aliquet orci,
> vitae pretium ante. Proin libero augue, vulputate eget iaculis vel,
> ultricies at velit. Phasellus euismod nisl vitae lacus scelerisque
> imperdiet sit amet vitae mi. Quisque rutrum leo et placerat iaculis.
> Fusce ac mattis eros, aliquet fringilla tellus. Quisque mauris mauris,
> dapibus nec orci et, lobortis tincidunt mauris.

Closed the old fd, the new fd is 4

(*) Writing:
+ Upon my word, man is created weaker and more base than you supposed! Can
+ he, can he perform the deeds of which you are capable? In respecting him
+ so much you acted as though you had ceased to have compassion for him,
+ because you demanded too much of him—and yet who was this? The very one
+ you had loved more than yourself! Had you respected him less you would
+ have demanded of him less, and that would have been closer to love, for
+ his burden would have been lighter. He is weak and dishonourable.
How many bytes were written: 499

(*) Read this data:
> Upon my word, man is created weaker and more base than you supposed! Can
> he, can he perform the deeds of which you are capable? In respecting him
> so much you acted as though you had ceased to have compassion for him,
> because you demanded too much of him—and yet who was this? The very one
> you had loved more than yourself! Had you respected him less you would
> have demanded of him less, and that would have been closer to love, for
> his burden would have been lighter. He is weak and dishonourable.

We killed the event manager and we're at the end
)";

std::stringstream OUTPUT{};

const uint8_t *get_write_data(const std::string &str) {
  return reinterpret_cast<const uint8_t *>(str.data());
}

std::string indent_with_str(const std::string &str, const std::string &indent) {
  std::string out{indent};
  for (const char c : str) {
    out += c;
    if (c == '\n') {
      out += indent;
    }
  }
  return out;
}

EvTask coro(EventManager *ev) {
  int fd = open("../test.txt", O_RDWR);

  char buff[2048]{};
  co_await ev->write(fd, get_write_data(lorem_ipsum), lorem_ipsum.length());

  co_await ev->read(fd, reinterpret_cast<uint8_t *>(buff), 2048);
  OUTPUT << "(*) Read this data:\n" << indent_with_str(buff, "> ") << "\n";

  co_await ev->close(fd);
  fd = open("../test.txt", O_RDWR | O_TRUNC);
  OUTPUT << "\nClosed the old fd, the new fd is " << fd << "\n\n";

  OUTPUT << "(*) Writing:\n"
         << indent_with_str(the_grand_inquisitor, "+ ") << "\n";
  auto res = co_await ev->write(fd, get_write_data(the_grand_inquisitor),
                                the_grand_inquisitor.length());
  OUTPUT << "How many bytes were written: " << res.data.bytes_wrote << "\n\n";

  std::memset(buff, 0, 2048);
  co_await ev->read(fd, reinterpret_cast<uint8_t *>(buff), 2048);
  OUTPUT << "(*) Read this data:\n" << indent_with_str(buff, "> ") << "\n\n";

  co_await ev->close(fd);

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  co_await ev->kill();

  OUTPUT << "We killed the event manager and we're at the end\n";
  co_return 0;
}

TEST_CASE("Testing variety of event loop capabilities") {
  EventManager ev(10);
  auto coroTask = coro(&ev);

  ev.register_coro(coroTask);
  ev.start();

  REQUIRE(OUTPUT.rdbuf()->str() == expected_output);
}