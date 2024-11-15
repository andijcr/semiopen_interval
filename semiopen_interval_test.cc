#include "semiopen_interval.hpp"
#include <boost/test/unit_test.hpp>
#include <concepts>
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <fmt/ranges.h>
#include <random>
#include <vector>

// define a semiregular K for testing, to ensure that we don't provide more
// operations than the library requires
struct IntK {
  int k;
  IntK() = default;
  IntK(int ik) noexcept : k(ik) {}
  IntK(IntK const &other) = default;
  IntK &operator=(IntK const &other) = default;
  IntK(IntK &&other) = default;
  IntK &operator=(IntK &&other) = default;

  friend constexpr bool operator<(IntK const &lhs, IntK const &rhs) {
    return lhs.k < rhs.k;
  }

  // print test helper, not needed by the library
  friend std::ostream &operator<<(std::ostream &os, IntK const &K) {
    return os << K.k;
  }
};
static_assert(std::semiregular<IntK>);
template <> struct fmt::formatter<IntK> : ostream_formatter {};

// define a regular V for testing, to ensure that we don't provide more
// operations than the library requires
struct CharV {
  char c;
  CharV(char vc) noexcept : c(vc) {}
  CharV() = default;
  CharV(CharV const &other) = default;
  CharV &operator=(CharV const &other) = default;
  CharV(CharV &&other) = default;
  CharV &operator=(CharV &&other) = default;

  friend constexpr bool operator==(CharV const &lhs, CharV const &rhs) {
    return lhs.c == rhs.c;
  }

  // print test helper
  friend std::ostream &operator<<(std::ostream &os, CharV const &v) {
    return os << v.c;
  }
};
static_assert(std::regular<CharV>);
template <> struct fmt::formatter<CharV> : ostream_formatter {};

struct semiopen_interval_tester {
  template <typename K, typename V>
  static auto access(semiopen_interval<K, V> const &in) {
    return std::tie(in._default_value, in._so_intervals);
  }
};

template <typename K, typename V>
void invariants_test(semiopen_interval<K, V> const &im) {
  // check invariants:
  // consecutive entries must not contain the same value.
  auto const &[def, map] = semiopen_interval_tester::access(im);
  auto prev = def;
  for (auto &[k, v] : map) {
    BOOST_CHECK_MESSAGE(prev != v,
                        fmt::format("repeated value {} at {}", v, k));
    prev = v;
  }
  // insertion in a new map of a range inserts 2 elements in the map
  BOOST_CHECK(map.empty() || map.size() >= 2);
  if (!map.empty()) {
    // (-inf, firstK) == [lastK, inf)
    BOOST_CHECK_EQUAL(def, std::prev(map.end())->second);
  }
}

template <typename K, typename V>
void invariants_test(semiopen_interval<K, V> const &im, bool) {
  auto const &[def, map] = semiopen_interval_tester::access(im);
  fmt::print("begin: {}, map: [{}]\n", def, fmt::join(map, ", "));
  invariants_test(im);
}

BOOST_AUTO_TEST_CASE(smoke_test) {
  auto im = semiopen_interval<IntK, CharV>{'A'};
  auto const &[def, map] = semiopen_interval_tester::access(im);
  BOOST_REQUIRE(def == 'A');
  BOOST_REQUIRE(map.empty());
  invariants_test(im);
}

BOOST_AUTO_TEST_CASE(test_assign_empty_simple) {
  auto im = semiopen_interval<IntK, CharV>{'A'};
  auto const &[def, map] = semiopen_interval_tester::access(im);
  im.assign(0, 0, CharV{'B'});
  BOOST_REQUIRE_EQUAL(def, CharV{'A'});
  BOOST_REQUIRE(map.empty());

  im.assign(0, -1, CharV{'B'});
  BOOST_REQUIRE_EQUAL(def, CharV{'A'});
  BOOST_REQUIRE(map.empty());
  invariants_test(im);
}

BOOST_AUTO_TEST_CASE(test_assign_simple) {
  auto im = semiopen_interval<IntK, CharV>{'A'};
  im.assign(0, 1, CharV{'A'});

  BOOST_REQUIRE_EQUAL(im[-1], 'A');
  BOOST_REQUIRE_EQUAL(im[0], 'A');
  BOOST_REQUIRE_EQUAL(im[1], 'A');
  invariants_test(im);
}

BOOST_AUTO_TEST_CASE(test_assign_simple_nv) {
  auto im = semiopen_interval<IntK, CharV>{'A'};
  im.assign(0, 1, CharV{'B'});

  BOOST_REQUIRE_EQUAL(im[-1], 'A');
  BOOST_CHECK_EQUAL(im[0], 'B');
  BOOST_REQUIRE_EQUAL(im[1], 'A');
  BOOST_REQUIRE_EQUAL(im[2], 'A');
  invariants_test(im);

  BOOST_TEST_CONTEXT("inserting a range before the map") {
    // expect no op
    auto [prev_def, prev_map] = semiopen_interval_tester::access(im);
    im.assign(-2, -1, CharV{'A'});
    auto const &[def, map] = semiopen_interval_tester::access(im);
    BOOST_CHECK(def == prev_def);
    BOOST_CHECK(map.size() == prev_map.size());

    // expect op
    im.assign(-2, -1, CharV{'B'});
    BOOST_CHECK_EQUAL(im[-3], 'A');
    BOOST_CHECK_EQUAL(im[-2], 'B');
    BOOST_CHECK_EQUAL(im[-1], 'A');
    BOOST_CHECK_EQUAL(im[0], 'B');
    BOOST_REQUIRE_EQUAL(im[1], 'A');

    invariants_test(im);
  }
  BOOST_TEST_CONTEXT("inserting a range after the map") {
    // expect no op
    auto [prev_def, prev_map] = semiopen_interval_tester::access(im);
    im.assign(2, 3, CharV{'A'});
    auto const &[def, map] = semiopen_interval_tester::access(im);
    BOOST_CHECK(def == prev_def);
    BOOST_CHECK(map.size() == prev_map.size());

    // expect op
    im.assign(2, 3, CharV{'B'});
    BOOST_CHECK_EQUAL(im[-3], 'A');
    BOOST_CHECK_EQUAL(im[-2], 'B');
    BOOST_CHECK_EQUAL(im[-1], 'A');
    BOOST_CHECK_EQUAL(im[0], 'B');
    BOOST_CHECK_EQUAL(im[1], 'A');
    BOOST_CHECK_EQUAL(im[2], 'B');
    BOOST_CHECK_EQUAL(im[3], 'A');
    BOOST_CHECK_EQUAL(im[4], 'A');

    invariants_test(im);
  }
}

BOOST_AUTO_TEST_CASE(test_assign_inside_simple) {
  auto im = semiopen_interval<IntK, CharV>{'A'};
  im.assign(0, 1, CharV{'B'});
  BOOST_CHECK_EQUAL(im[-1], 'A');
  BOOST_CHECK_EQUAL(im[0], 'B');
  BOOST_CHECK_EQUAL(im[1], 'A');
  invariants_test(im, true);
  im.assign(0, 1, CharV{'C'});
  BOOST_CHECK_EQUAL(im[-1], 'A');
  BOOST_CHECK_EQUAL(im[0], 'C');
  BOOST_CHECK_EQUAL(im[1], 'A');
  invariants_test(im, true);

  im.assign(-1, 1, CharV{'D'});
  BOOST_CHECK_EQUAL(im[-2], 'A');
  BOOST_CHECK_EQUAL(im[-1], 'D');
  BOOST_CHECK_EQUAL(im[0], 'D');
  BOOST_CHECK_EQUAL(im[1], 'A');
  invariants_test(im, true);

  im.assign(-1, 0, CharV{'E'});
  BOOST_CHECK_EQUAL(im[-2], 'A');
  BOOST_CHECK_EQUAL(im[-1], 'E');
  BOOST_CHECK_EQUAL(im[0], 'D');
  BOOST_CHECK_EQUAL(im[1], 'A');
  invariants_test(im, true);

  im.assign(-1, 0, CharV{'A'});
  BOOST_CHECK_EQUAL(im[-2], 'A');
  BOOST_CHECK_EQUAL(im[-1], 'A');
  BOOST_CHECK_EQUAL(im[0], 'D');
  BOOST_CHECK_EQUAL(im[1], 'A');
  invariants_test(im, true);

  im.assign(0, 10, CharV{'A'});
  BOOST_CHECK_EQUAL(im[-2], 'A');
  BOOST_CHECK_EQUAL(im[-1], 'A');
  BOOST_CHECK_EQUAL(im[0], 'A');
  BOOST_CHECK_EQUAL(im[1], 'A');
  invariants_test(im, true);
}

struct a_move {
  IntK beg;
  IntK end;
  CharV val;
  friend std::ostream &operator<<(std::ostream &os, a_move const &m) {
    return os << fmt::format("(b:{}, e:{}, v:{})", m.beg, m.end, m.val);
  }
};

template <> struct fmt::formatter<a_move> : ostream_formatter {};

BOOST_AUTO_TEST_CASE(test_critical_seq) {
  auto im = semiopen_interval<IntK, CharV>{'A'};
  im.assign(39, 42, CharV{'C'});
  invariants_test(im, true);
  im.assign(42, 89, CharV{'C'});
  invariants_test(im, true);
}

BOOST_AUTO_TEST_CASE(test_some_random_ops) {
  auto rd = std::random_device{};
  auto gen = std::mt19937{rd()};
  auto keys = std::uniform_int_distribution<int>(0, 10);
  auto vals = std::uniform_int_distribution<char>('A', 'D');

  for (auto j = 0; j < 10; ++j) {
    auto im = semiopen_interval<IntK, CharV>{vals(gen)};
    auto moves = std::vector<a_move>{};
    // run some random operations
    for (auto i = std::uniform_int_distribution<size_t>(2, 250)(gen); i > 0;
         --i) {
      auto const &[b, e, v] = moves.emplace_back(
          IntK{keys(gen)}, IntK{keys(gen)}, CharV{vals(gen)});
      im.assign(b, e, v);
      BOOST_TEST_CONTEXT(
          fmt::format("after {} moves", fmt::join(moves, ", "))) {
        invariants_test(im, true);
      }
    }

    // at the end try some operations that will overwrite the whole range
    for (auto i = 0; i < 10; ++i) {
      im.assign(-200, 200, CharV{vals(gen)});
      invariants_test(im, true);
      im.assign(200, 210, CharV{vals(gen)});
      invariants_test(im, true);
      im.assign(240, 241, CharV{vals(gen)});
      invariants_test(im, true);
    }
  }
}