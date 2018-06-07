#include <gtest/gtest.h>

#include "dimensions.h"

TEST(Dimensions, count_and_volume) {
  Dimensions dims;
  EXPECT_EQ(dims.count(), 0);
  EXPECT_EQ(dims.volume(), 1);
  dims.add(Dimension::Tof, 3);
  EXPECT_EQ(dims.count(), 1);
  EXPECT_EQ(dims.volume(), 3);
  dims.add(Dimension::Q, 2);
  EXPECT_EQ(dims.count(), 2);
  EXPECT_EQ(dims.volume(), 6);
}

TEST(Dimensions, offset) {
  Dimensions dims;
  dims.add(Dimension::Tof, 3);
  dims.add(Dimension::Q, 2);
  EXPECT_EQ(dims.offset(Dimension::Tof), 1);
  EXPECT_EQ(dims.offset(Dimension::Q), 3);
}
