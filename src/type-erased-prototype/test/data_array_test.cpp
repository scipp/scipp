#include <gtest/gtest.h>
#include <vector>

#include "data_array.h"

TEST(DataArray, construct) {
  ASSERT_NO_THROW(DataArray a(0, std::vector<double>(2)));
}

TEST(DataArray, sharing) {
  const DataArray a1(0, std::vector<double>(2));
  const auto a2(a1);
  ASSERT_EQ(&a1.cast<std::vector<double>>(), &a2.cast<std::vector<double>>());
}

TEST(DataArray, copy) {
  const DataArray a1(0, std::vector<double>(2));
  auto a2(a1);
  // Calls the non-const cast version, so a2 make a copy.
  ASSERT_NE(&a1.cast<std::vector<double>>(), &a2.cast<std::vector<double>>());
}
