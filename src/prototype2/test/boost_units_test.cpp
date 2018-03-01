#include <gtest/gtest.h>

#include "units.h"

TEST(BoostUnits, add) {
  UnitId a{UnitId::Dimensionless};
  UnitId b{UnitId::Length};
  UnitId c{UnitId::Area};
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

TEST(BoostUnits, multiply) {
  UnitId a{UnitId::Dimensionless};
  UnitId b{UnitId::Length};
  UnitId c{UnitId::Area};
  EXPECT_EQ(a * a, a);
  EXPECT_EQ(a * b, b);
  EXPECT_EQ(b * a, b);
  EXPECT_EQ(a * c, c);
  EXPECT_EQ(c * a, c);
  EXPECT_EQ(b * b, c);
  EXPECT_ANY_THROW(b * c);
  EXPECT_ANY_THROW(c * b);
  EXPECT_ANY_THROW(c * c);
}
