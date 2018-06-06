#include <gtest/gtest.h>
#include <vector>

#include "data_array.h"

TEST(DataArray, construct) {
  ASSERT_NO_THROW(DataArray a(0, std::vector<double>(2)));
}
