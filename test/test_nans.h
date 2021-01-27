#pragma once
#include <gtest/gtest.h>
#include <numeric>
#include <type_traits>

namespace {
template <typename T> struct MeanTest : public ::testing::Test {
  using TestType = T;
  /**
   * mean and nanmean will preserve input type for floating point types.
   * For any integer input type we expect the return type to be double
   */
  using ReturnType = std::conditional_t<std::is_integral_v<T>, double, T>;
  /// Held type supports nan testing.
  constexpr static bool TestNans = std::numeric_limits<T>::has_quiet_NaN;
  /// Held type supports variances testing. All FP types -> true
  constexpr static bool TestVariances = std::is_floating_point_v<T>;
};
} // namespace
