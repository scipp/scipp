/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include "unit.h"

TEST(Unit, construct) { ASSERT_NO_THROW(Unit u{Unit::Id::Dimensionless}); }

TEST(Unit, compare) {
  Unit u1{Unit::Id::Dimensionless};
  Unit u2{Unit::Id::Length};
  ASSERT_TRUE(u1 == u1);
  ASSERT_TRUE(u1 != u2);
}

TEST(Unit, add) {
  Unit a{Unit::Id::Dimensionless};
  Unit b{Unit::Id::Length};
  Unit c{Unit::Id::Area};
  EXPECT_EQ(a + a, a);
  EXPECT_EQ(b + b, b);
  EXPECT_EQ(c + c, c);
  EXPECT_ANY_THROW(a + b);
  EXPECT_ANY_THROW(a + c);
  EXPECT_ANY_THROW(b + a);
  EXPECT_ANY_THROW(b + c);
  EXPECT_ANY_THROW(c + a);
  EXPECT_ANY_THROW(c + b);
}

TEST(Unit, multiply) {
  Unit a{Unit::Id::Dimensionless};
  Unit b{Unit::Id::Length};
  Unit c{Unit::Id::Area};
  EXPECT_EQ(a * a, a);
  EXPECT_EQ(a * b, b);
  EXPECT_EQ(b * a, b);
  EXPECT_EQ(a * c, c);
  EXPECT_EQ(c * a, c);
  EXPECT_EQ(b * b, c);
  EXPECT_ANY_THROW(b * c);
  EXPECT_ANY_THROW(c * b);
  EXPECT_EQ(c * c, Unit::Id::AreaVariance);
}

TEST(Unit, multiply_counts) {
  Unit counts{Unit::Id::Counts};
  Unit none{Unit::Id::Dimensionless};
  EXPECT_EQ(counts * counts, Unit::Id::CountsVariance);
  EXPECT_EQ(counts * none, counts);
  EXPECT_EQ(none * counts, counts);
}
