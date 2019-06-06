// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/units/unit.h"

using namespace scipp;
using namespace scipp::units;

TEST(DummyUnitsTest, basics) {
  // Current neutron::Unit is inlined as Unit, but we can still use others.
  dummy::Unit m{units::m};
  dummy::Unit s{units::s};
  ASSERT_NE(m, s);
  dummy::Unit expected{units::m / units::s};
  auto result = m / s;
  EXPECT_EQ(result, expected);
}

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
  EXPECT_EQ(counts * none, counts);
  EXPECT_EQ(none * counts, counts);
}

TEST(Unit, conversion_factors) {
  boost::units::quantity<detail::tof::wavelength> a(2.0 * angstrom);
  boost::units::quantity<boost::units::si::length> b(3.0 * angstrom);
  boost::units::quantity<detail::tof::wavelength> c(4.0 *
                                                    boost::units::si::meters);
  boost::units::quantity<boost::units::si::area> d(
      5.0 * boost::units::si::meters * angstrom);
  boost::units::quantity<detail::tof::energy> e = 6.0 * meV;
  boost::units::quantity<boost::units::si::energy> f(7.0 * meV);
  boost::units::quantity<boost::units::si::time> g(8.0 * us);
  boost::units::quantity<detail::tof::tof> h(9.0 * boost::units::si::seconds);
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

TEST(Unit, isCounts) {
  EXPECT_FALSE(Unit(units::dimensionless).isCounts());
  EXPECT_TRUE(Unit(units::counts).isCounts());
  EXPECT_FALSE(Unit(units::counts / units::us).isCounts());
  EXPECT_FALSE(Unit(units::counts / units::meV).isCounts());
  EXPECT_FALSE(Unit(units::dimensionless / units::m).isCounts());
}

TEST(Unit, isCountDensity) {
  EXPECT_FALSE(Unit(units::dimensionless).isCountDensity());
  EXPECT_FALSE(Unit(units::counts).isCountDensity());
  EXPECT_TRUE(Unit(units::counts / units::us).isCountDensity());
  EXPECT_TRUE(Unit(units::counts / units::meV).isCountDensity());
  EXPECT_FALSE(Unit(units::dimensionless / units::m).isCountDensity());
}

TEST(DummyUnitsTest, isCounts) {
  EXPECT_TRUE(dummy::Unit(units::dimensionless).isCounts());
  EXPECT_FALSE(dummy::Unit(units::dimensionless / units::m).isCounts());
}

TEST(DummyUnitsTest, isCountDensity) {
  EXPECT_FALSE(dummy::Unit(units::dimensionless).isCountDensity());
  EXPECT_TRUE(dummy::Unit(units::dimensionless / units::m).isCountDensity());
}
