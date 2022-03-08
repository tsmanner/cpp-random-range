#pragma once

#include <algorithm>
#include <random>
#include <vector>

template <typename T>
class random_range {
public:
  using result_type = T;
  using value_type = std::pair<result_type, result_type>;
  using allocator_type = typename std::vector<value_type>::allocator_type;

  random_range() {}
  random_range(result_type const &inMin, result_type const &inMax): mRanges({{inMin, inMax}}) { updateWeights(); }
  random_range(value_type const &inRange): mRanges({inRange}) { updateWeights(); }
  random_range(std::vector<value_type> const &inRanges): mRanges(inRanges) { updateWeights(); }

  template <class InputIt>
  random_range(InputIt first, InputIt last, allocator_type const &alloc = allocator_type()): mRanges(first, last, alloc) { updateWeights(); }

  // Copy
  random_range(random_range const &other): mRanges(other.mRanges), mEngine(other.mEngine), mDistribution(other.mDistribution) {}

  // Move
  random_range(random_range &&other): mRanges(other.mRanges), mEngine(other.mEngine), mDistribution(other.mDistribution) {}

  // Return a value, leaving it in the set of possibilities
  result_type selectWithReplacement() const {
    if (size() == 0) {
      throw std::out_of_range("Attempt to draw a value from an empty random_range!");
    }
    // Select a range by weight
    auto &range = mRanges[mDistribution(mEngine)];
    // Select from that range using a uniform distribution
    return std::uniform_int_distribution<result_type> {range.first, range.second}(mEngine);
  }

  // Return a value, removing it from the set of possibilities
  result_type selectWithoutReplacement() {
    auto value = selectWithReplacement();
    remove(value);
    return value;
  }

  // Put a value back in the set of possibilities
  void replace(result_type const &value) {
    // Find the first range that starts higher than value
    typename decltype(mRanges)::iterator higherIter;
    typename decltype(mRanges)::iterator lowerIter;
    std::tie(higherIter, lowerIter) = [&] () -> value_type {
      auto higher = std::find_if(mRanges.begin(), mRanges.end(), [&](value_type const &inRange) { return value < inRange.first; });
      auto lower = higher;
      if (higher != mRanges.begin()) {
        --lower;
      }
      return {higher, lower};
    }();
    // Figure out if it borders the higher and/or lower range(s)
    bool bordersRangeAbove = higherIter != mRanges.end() and value == std::get<0>(*higherIter) - 1;
    bool bordersRangeBelow = lowerIter != higherIter and value == std::get<1>(*lowerIter) + 1;
    // Borders both - extend lower to cover higher, and erase higher
    if (bordersRangeAbove and bordersRangeBelow) {
      lowerIter->second = higherIter->second;
      mRanges.erase(higherIter);
    }
    // Borders only higher range - decrement the higher range's min
    else if (bordersRangeAbove) {
      --higherIter->first;
    }
    // Borders only lower range - increment the lower range's max
    else if (bordersRangeBelow) {
      ++higherIter->second;
    }
    // Borders neither range - insert a new 1-value range
    else {
      mRanges.insert(higherIter, {value, value});
    }
  }

  // Remove a value from the set of possibilities
  void remove(result_type const &value) {
    for (auto iter = mRanges.begin(); iter != mRanges.end(); ++iter) {
      // If the drawn value is this range's lower bound
      if (iter->first == value) {
        // If the range is only 1 wide, just erase it and break
        if (iter->first == iter->second) {
          mRanges.erase(iter);
          break;
        }
        // If the range is more than 1 wide, just bump the min up
        else {
          ++iter->first;
        }
      }
      // If the drawn value is this range's upper bound
      else if (value == iter->second) {
        // If the range is only 1 wide, just erase it and break
        if (iter->first == iter->second) {
          mRanges.erase(iter);
          break;
        }
        // If the range is more than 1 wide, just drop the max down
        else {
          --iter->second;
        }
      }
      // If the drawn value lies within this range, split it and break
      else if (iter->first < value and value < iter->second) {
        value_type lower {iter->first, value - 1};
        value_type upper {value + 1, iter->second};
        // Returns an iterator to the new next element or end()
        iter = mRanges.erase(iter);
        mRanges.insert(iter, {lower, upper});
        break;
      }
    }
    updateWeights();
  }

  // Return the number of values in the set of possibilities
  std::size_t size() const {
    std::size_t mySize = 0;
    for (auto &range : mRanges) {
      mySize += size(range);
    }
    return mySize;
  }

  result_type min() const {
    return mRanges.front().first;
  }

  result_type max() const {
    return mRanges.back().second;
  }

  std::vector<value_type> const &ranges() const {
    return mRanges;
  }

  std::vector<double> weights() const {
    return mDistribution.probabilities();
  }

  void updateWeights() const {
    // Set mDistribution for each range based on the range size
    std::vector<double> weights;
    weights.reserve(mRanges.size());
    for (auto &range : mRanges) {
      weights.push_back(size(range));
    }
    mDistribution = decltype(mDistribution)(weights.begin(), weights.end());
  }

  void seed(T const &value = std::mt19937::default_seed) {
    mEngine.seed(value);
  }

  template <class Sseq>
  void seed(Sseq &seq) {
    mEngine.seed(seq);
  }

private:
  std::vector<value_type> mRanges {};
  std::mt19937 mutable mEngine {};
  std::discrete_distribution<std::size_t> mutable mDistribution {};

  std::size_t static inline size(value_type const &range) {
    return range.second - range.first + 1;
  }

};
