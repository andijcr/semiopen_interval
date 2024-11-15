#include "semiopen_interval.hpp"
#include <cstdint>
#include <cstring>
#include <span>

static_assert(__has_feature(address_sanitizer),
              "it's useless to compile this fuzzer without a sanitizer");

// empty tape stop exception
struct stop_exception : public std::exception {};

// interpret a buffer as a sequence of char and ints
struct tape {

  std::span<const uint8_t> buf;

  char pull_char() {
    if (buf.empty()) {
      throw stop_exception{};
    }
    auto ch = static_cast<char>(buf.front());
    buf = buf.subspan(1);
    return ch;
  }

  int pull_int() {
    if (buf.size() < sizeof(int)) {
      throw stop_exception{};
    }
    int i;
    std::memcpy(&i, buf.data(), sizeof(i));
    buf = buf.subspan(sizeof(i));
    return i;
  }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  // fuzzer test: apply a random sequence of assign operation, see if anything
  // triggers a sanitizer

  auto input = tape{
      .buf = {Data, Size},
  };

  try {
    // init
    auto map = semiopen_interval<int, char>{input.pull_char()};

    while (true) {
      // random op
      map.assign(input.pull_int(), input.pull_int(), input.pull_char());
    }
  } catch (stop_exception const &) {
    // normal stop, no more input
  }
  return 0;
}