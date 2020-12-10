#pragma once
#include <gtest/gtest.h>
#include <numeric>
#include <type_traits>

namespace {
template <typename T> struct MeanTest : public ::testing::Test {
  /**
   * mean and nanmean will preserve input type for floating point types.
   * For any integer input type we expect the return type to be double
   */
  using RetType =
      std::conditional_t<std::numeric_limits<T>::is_integer, double, T>;
  /// Held type supports nan testing. All FP types -> true
  constexpr static bool TestNans = !std::numeric_limits<T>::is_integer;
  /// Held type supports variances testing. All FP types -> true
  constexpr static bool TestVariances = !std::numeric_limits<T>::is_integer;
};
} // namespace
