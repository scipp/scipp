/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include "bool.h"

TEST(Bool, size) { EXPECT_EQ(sizeof(Bool), 1ul); }

TEST(Bool, std_vector_initializer_list) {
  std::vector<Bool> bs{true, false};
  EXPECT_EQ(bs[0], true);
  EXPECT_EQ(bs[1], false);
}

TEST(Bool, avoids_std_vector_specialization) {
  std::vector<Bool> bytes[9];
  EXPECT_EQ(&bytes[0] + 8, &bytes[8]);

  // For comparison, the following *may* fail (bool specialization is not
  // *guaranteed* by the standard, so we cannot actually assert on this).
  // std::vector<bool> packed[9];
  // EXPECT_EQ(&packed[0] + 8, &packed[8]);
}

TEST(Bool, basics) {
  Bool b;
  EXPECT_FALSE(b);
  EXPECT_TRUE(Bool(true));
  EXPECT_EQ(Bool(false), false);
  EXPECT_EQ(Bool(true), true);
  EXPECT_NE(Bool(false), true);
  EXPECT_NE(Bool(true), false);
}

TEST(Bool, assign_to_bool) {
  bool b = Bool(true);
  EXPECT_TRUE(b);
  b = Bool(false);
  EXPECT_FALSE(b);
}

TEST(Bool, assign_from_bool) {
  Bool b = true;
  EXPECT_TRUE(b);
  b = false;
  EXPECT_FALSE(b);
}
