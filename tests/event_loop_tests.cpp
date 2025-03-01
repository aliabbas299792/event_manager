#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "vendor/doctest/doctest/doctest.h"

#include "event_manager.hpp"
#include <fcntl.h>
#include <thread>

const std::string LOREM_IPSUM = R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean ultricies
ex sit amet orci tincidunt, a viverra sem suscipit. Phasellus non quam
bibendum, molestie enim vel, placerat nibh. Mauris elementum, nisi et
fringilla auctor, velit justo porttitor neque, ut viverra neque magna
quis urna. Mauris eu libero lacinia, pulvinar purus nec, dapibus mi.
Aliquam id accumsan nunc, vitae molestie diam. Proin eget aliquet orci,
vitae pretium ante. Proin libero augue, vulputate eget iaculis vel,
ultricies at velit. Phasellus euismod nisl vitae lacus scelerisque
imperdiet sit amet vitae mi. Quisque rutrum leo et placerat iaculis.
Fusce ac mattis eros, aliquet fringilla tellus. Quisque mauris mauris,
dapibus nec orci et, lobortis tincidunt mauris.)";

const std::string THE_GRAND_INQUISITOR =
    R"(Upon my word, man is created weaker and more base than you supposed! Can
he, can he perform the deeds of which you are capable? In respecting him
so much you acted as though you had ceased to have compassion for him,
because you demanded too much of him—and yet who was this? The very one
you had loved more than yourself! Had you respected him less you would
have demanded of him less, and that would have been closer to love, for
his burden would have been lighter. He is weak and dishonourable.)";

const std::string EXPECTED_OUTPUT = R"((*) Read this data:
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

std::stringstream output{};

const uint8_t* get_write_data(const std::string& str) {
  return reinterpret_cast<const uint8_t*>(str.data());
}

std::string indent_with_str(const std::string& str, const std::string& indent) {
  std::string out{indent};
  for (const char C : str) {
    out += C;
    if (C == '\n') {
      out += indent;
    }
  }
  return out;
}

EvTask coro(EventManager* ev) {
  auto filepath = "./test.txt";

  int fd = open(filepath, O_RDWR | O_CREAT, 0666);

  char buff[2048]{};
  co_await ev->write(fd, get_write_data(LOREM_IPSUM), LOREM_IPSUM.length());

  co_await ev->read(fd, reinterpret_cast<uint8_t*>(buff), 2048);
  output << "(*) Read this data:\n" << indent_with_str(buff, "> ") << "\n";

  co_await ev->close(fd);
  fd = open(filepath, O_RDWR | O_TRUNC);

  output << "\n(*) Writing:\n" << indent_with_str(THE_GRAND_INQUISITOR, "+ ") << "\n";
  auto res = co_await ev->write(fd, get_write_data(THE_GRAND_INQUISITOR), THE_GRAND_INQUISITOR.length());
  output << "How many bytes were written: " << res.data.bytes_wrote << "\n\n";

  std::memset(buff, 0, 2048);
  co_await ev->read(fd, reinterpret_cast<uint8_t*>(buff), 2048);
  output << "(*) Read this data:\n" << indent_with_str(buff, "> ") << "\n\n";

  co_await ev->close(fd);

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  co_await ev->kill();

  output << "We killed the event manager and we're at the end\n";
  co_return 0;
}

TEST_CASE("Testing variety of event loop capabilities") {
  EventManager ev(10);

  ev.register_coro(coro(&ev));
  ev.start();

  REQUIRE(output.rdbuf()->str() == EXPECTED_OUTPUT);
}