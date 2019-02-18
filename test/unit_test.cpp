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

TEST(Unit, conversion_factors) {
  boost::units::quantity<datasetunits::wavelength> a(2.0 *
                                                     datasetunits::lambdas);
  boost::units::quantity<boost::units::si::length> b(3.0 *
                                                     datasetunits::lambdas);
  boost::units::quantity<datasetunits::wavelength> c(4.0 *
                                                     boost::units::si::meters);
  boost::units::quantity<boost::units::si::area> d(
      5.0 * boost::units::si::meters * datasetunits::lambdas);
  boost::units::quantity<datasetunits::energy> e = 6.0 * datasetunits::meV;
  boost::units::quantity<boost::units::si::energy> f(7.0 * datasetunits::meV);
  boost::units::quantity<boost::units::si::time> g(8.0 *
                                                   datasetunits::microseconds);
  boost::units::quantity<datasetunits::tof> h(9.0 * boost::units::si::seconds);
  EXPECT_DOUBLE_EQ(a.value(), 2.0);
  EXPECT_DOUBLE_EQ(b.value(), 3.0e-10);
  EXPECT_DOUBLE_EQ(c.value(), 4.0e10);
  EXPECT_DOUBLE_EQ(d.value(), 5.0e-10);
  EXPECT_DOUBLE_EQ(e.value(), 6.0);
  EXPECT_DOUBLE_EQ(f.value(),
                   7.0e-3 *
                       boost::units::si::constants::codata::e.value().value());
  EXPECT_DOUBLE_EQ(g.value(), 8.0e-6);
  EXPECT_DOUBLE_EQ(h.value(), 9.0e6);
}