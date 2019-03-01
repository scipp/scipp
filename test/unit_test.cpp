/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include "test_macros.h"

#include "unit.h"

TEST(units, c) {
  auto c = 1.0 * units::c;
  EXPECT_EQ(c.value(), 1.0);

  boost::units::quantity<boost::units::si::velocity> si_c(c);
  EXPECT_EQ(si_c.value(), 299792458.0);
}

TEST(Unit, construct) { ASSERT_NO_THROW(Unit u{units::dimensionless}); }

TEST(Unit, compare) {
  Unit u1{units::dimensionless};
  Unit u2{units::m};
  ASSERT_TRUE(u1 == u1);
  ASSERT_TRUE(u1 != u2);
}

TEST(Unit, add) {
  Unit a{units::dimensionless};
  Unit b{units::m};
  Unit c{units::m * units::m};
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
  Unit a{units::dimensionless};
  Unit b{units::m};
  Unit c{units::m * units::m};
  EXPECT_EQ(a * a, a);
  EXPECT_EQ(a * b, b);
  EXPECT_EQ(b * a, b);
  EXPECT_EQ(a * c, c);
  EXPECT_EQ(c * a, c);
  EXPECT_EQ(b * b, c);
  EXPECT_ANY_THROW(b * c);
  EXPECT_ANY_THROW(c * b);
  EXPECT_EQ(c * c, units::m * units::m * units::m * units::m);
}

TEST(Unit, multiply_counts) {
  Unit counts{units::counts};
  Unit none{units::dimensionless};
  EXPECT_EQ(counts * counts, units::counts * units::counts);
  EXPECT_EQ(counts * none, counts);
  EXPECT_EQ(none * counts, counts);
}

TEST(Unit, conversion_factors) {
  boost::units::quantity<neutron::tof::wavelength> a(2.0 *
                                                     neutron::tof::angstroms);
  boost::units::quantity<boost::units::si::length> b(3.0 *
                                                     neutron::tof::angstroms);
  boost::units::quantity<neutron::tof::wavelength> c(4.0 *
                                                     boost::units::si::meters);
  boost::units::quantity<boost::units::si::area> d(
      5.0 * boost::units::si::meters * neutron::tof::angstroms);
  boost::units::quantity<neutron::tof::energy> e = 6.0 * neutron::tof::meV;
  boost::units::quantity<boost::units::si::energy> f(7.0 * neutron::tof::meV);
  boost::units::quantity<boost::units::si::time> g(8.0 *
                                                   neutron::tof::microseconds);
  boost::units::quantity<neutron::tof::tof> h(9.0 * boost::units::si::seconds);
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

TEST(Unit, c) {
  Unit c(units::c);
  EXPECT_EQ(c * Unit(units::m), Unit(units::c * units::m));
  EXPECT_EQ(c * Unit(units::m) / Unit(units::m), Unit(units::c));
  EXPECT_EQ(Unit(units::meV) / c, Unit(units::meV / units::c));
  EXPECT_EQ(Unit(units::meV) / c / Unit(units::meV),
            Unit(units::dimensionless / units::c));
}

TEST(Unit, sqrt) {
  Unit a{units::dimensionless};
  Unit m{units::m};
  Unit m2{units::m * units::m};
  EXPECT_EQ(sqrt(m2), m);
}

TEST(Unit, sqrt_fail) {
  Unit m{units::m};
  EXPECT_THROW_MSG(sqrt(m), std::runtime_error,
                   "Unsupported unit as result of sqrt: sqrt(m).");
}
