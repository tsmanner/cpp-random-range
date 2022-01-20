#include <map>
#include <set>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "include/random_range.h"



template <typename T>
void checkInternals(random_range<T> const &inSelect) {
  if (inSelect.ranges().size()) {
    CHECK(inSelect.ranges().size() == inSelect.weights().size());
    for (auto &weight : inSelect.weights()) {
      CHECK(weight > 0.0);
    }
  }
  else {
    CHECK(inSelect.weights().size() == 1);
  }
}

TEST_CASE("With Replacement") {
  random_range<int> range{-1, 5};
  checkInternals(range);
  for (unsigned _ = 0; _ < 100; ++_) {
    auto value = range.selectWithReplacement();
    CHECK(range.min() <= value);
    CHECK(value <= range.max());
    checkInternals(range);
  }
}

TEST_CASE("Without Replacement") {
  random_range<int> range{-1, 5};
  checkInternals(range);
  std::set<typename decltype(range)::result_type> values;
  unsigned iterations = range.size();
  for (unsigned _ = 0; _ < iterations; ++_) {
    typename decltype(range)::result_type min = range.min();
    typename decltype(range)::result_type max = range.max();
    auto value = range.selectWithoutReplacement();
    CHECK(min <= value);
    CHECK(value <= max);
    // Check that the value was actually inserted, meaning it wasn't drawn already
    CHECK(values.insert(value).second);
    checkInternals(range);
  }
  CHECK(range.size() == 0);
}

template <typename T>
void checkDistribution(random_range<T> const &range, unsigned const &drawsPerValue, double const &epsilon) {
  std::map<typename random_range<T>::result_type, unsigned> counts;
  for (unsigned _ = 0; _ < range.size() * drawsPerValue; ++_) {
    ++counts[range.selectWithReplacement()];
  }
  // Assert that drawing a whole bunch of times appears to show
  // a uniform distribution.
  CHECK(range.size() == counts.size());
  for (auto &p : counts) {
    CHECK(p.second == Approx(drawsPerValue).epsilon(epsilon));
  }
}

TEST_CASE("Distribution") {
  random_range<int> range{1, 100};
  static constexpr unsigned drawsPerValue = 10000;
  // Check the distribution of results, allowing for ±10% variance from
  // the expected number of draws per value.
  checkDistribution(range, drawsPerValue, 0.1);
  // Modify the range, and check the distribution again.
  range.selectWithoutReplacement();
  checkDistribution(range, drawsPerValue, 0.1);
}
