// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/units/except.h"
#include "scipp/units/unit.h"

using namespace scipp;
using scipp::units::Unit;

TEST(UnitTest, constants) {
  EXPECT_EQ(units::dimensionless, Unit(units::boost_units::dimensionless));
  EXPECT_EQ(units::one, Unit(units::boost_units::dimensionless));
  EXPECT_EQ(units::m, Unit(units::boost_units::m));
  EXPECT_EQ(units::s, Unit(units::boost_units::s));
  EXPECT_EQ(units::kg, Unit(units::boost_units::kg));
  EXPECT_EQ(units::K, Unit(units::boost_units::K));
  EXPECT_EQ(units::rad, Unit(units::boost_units::rad));
  EXPECT_EQ(units::deg, Unit(units::boost_units::deg));
  EXPECT_EQ(units::angstrom, Unit(units::boost_units::angstrom));
  EXPECT_EQ(units::meV, Unit(units::boost_units::meV));
  EXPECT_EQ(units::us, Unit(units::boost_units::us));
  EXPECT_EQ(units::c, Unit(units::boost_units::c));
}

TEST(UnitTest, c) {
  auto c = 1.0 * units::boost_units::c;
  EXPECT_EQ(c.value(), 1.0);

  boost::units::quantity<boost::units::si::velocity> si_c(c);
  EXPECT_EQ(si_c.value(), 299792458.0);
}

TEST(UnitTest, cancellation) {
  EXPECT_EQ(Unit(units::deg / units::deg), units::dimensionless);
  EXPECT_EQ(units::deg / units::deg, units::dimensionless);
  EXPECT_EQ(units::deg * Unit(units::rad / units::deg), units::rad);
}

TEST(UnitTest, construct) { ASSERT_NO_THROW(Unit u{units::dimensionless}); }

TEST(UnitTest, construct_default) {
  Unit u;
  ASSERT_EQ(u, units::dimensionless);
}

TEST(UnitTest, compare) {
  Unit u1{units::dimensionless};
  Unit u2{units::m};
  ASSERT_TRUE(u1 == u1);
  ASSERT_TRUE(u1 != u2);
  ASSERT_TRUE(u2 == u2);
  ASSERT_FALSE(u1 == u2);
  ASSERT_FALSE(u2 != u2);
}

TEST(UnitTest, add) {
  Unit a{units::dimensionless};
  Unit b{units::m};
  Unit c{units::m * units::m};
  EXPECT_EQ(a + a, a);
  EXPECT_EQ(b + b, b);
  EXPECT_EQ(c + c, c);
  EXPECT_THROW(a + b, except::UnitError);
  EXPECT_THROW(a + c, except::UnitError);
  EXPECT_THROW(b + a, except::UnitError);
  EXPECT_THROW(b + c, except::UnitError);
  EXPECT_THROW(c + a, except::UnitError);
  EXPECT_THROW(c + b, except::UnitError);
}

TEST(UnitTest, multiply) {
  Unit a{units::dimensionless};
  Unit b{units::m};
  Unit c{units::m * units::m};
  EXPECT_EQ(a * a, a);
  EXPECT_EQ(a * b, b);
  EXPECT_EQ(b * a, b);
  EXPECT_EQ(a * c, c);
  EXPECT_EQ(c * a, c);
  EXPECT_EQ(b * b, c);
  EXPECT_EQ(b * c, units::m * units::m * units::m);
  EXPECT_EQ(c * b, units::m * units::m * units::m);
  EXPECT_THROW(c * c, except::UnitError);
}

TEST(UnitTest, multiply_counts) {
  Unit counts{units::counts};
  Unit none{units::dimensionless};
  EXPECT_EQ(counts * none, counts);
  EXPECT_EQ(none * counts, counts);
}

TEST(UnitTest, divide) {
  Unit one{units::dimensionless};
  Unit l{units::m};
  Unit t{units::s};
  Unit v{units::m / units::s};
  EXPECT_EQ(l / one, l);
  EXPECT_EQ(t / one, t);
  EXPECT_EQ(l / l, one);
  EXPECT_EQ(l / t, v);
  EXPECT_THROW(one / v, except::UnitError);
}

TEST(UnitTest, divide_counts) {
  Unit counts{units::counts};
  EXPECT_EQ(counts / counts, units::dimensionless);
}

TEST(UnitTest, conversion_factors) {
  boost::units::quantity<units::detail::tof::wavelength> a(
      2.0 * units::boost_units::angstrom);
  boost::units::quantity<boost::units::si::length> b(
      3.0 * units::boost_units::angstrom);
  boost::units::quantity<units::detail::tof::wavelength> c(
      4.0 * boost::units::si::meters);
  boost::units::quantity<boost::units::si::area> d(
      5.0 * boost::units::si::meters * units::boost_units::angstrom);
  boost::units::quantity<units::detail::tof::energy> e =
      6.0 * units::boost_units::meV;
  boost::units::quantity<boost::units::si::energy> f(7.0 *
                                                     units::boost_units::meV);
  boost::units::quantity<boost::units::si::time> g(8.0 *
                                                   units::boost_units::us);
  boost::units::quantity<units::detail::tof::tof> h(9.0 *
                                                    boost::units::si::seconds);
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

TEST(UnitTest, neutron_units) {
  Unit c(units::c);
  EXPECT_EQ(c * units::m, Unit(units::c * units::m));
  EXPECT_EQ(c * units::m / units::m, units::c);
  EXPECT_EQ(units::meV / c, Unit(units::meV / units::c));
  EXPECT_EQ(units::meV / c / units::meV, Unit(units::dimensionless / units::c));
}

TEST(UnitTest, isCounts) {
  EXPECT_FALSE(units::dimensionless.isCounts());
  EXPECT_TRUE(units::counts.isCounts());
  EXPECT_FALSE(Unit(units::counts / units::us).isCounts());
  EXPECT_FALSE(Unit(units::counts / units::meV).isCounts());
  EXPECT_FALSE(Unit(units::dimensionless / units::m).isCounts());
}

TEST(UnitTest, isCountDensity) {
  EXPECT_FALSE(units::dimensionless.isCountDensity());
  EXPECT_FALSE(units::counts.isCountDensity());
  EXPECT_TRUE(Unit(units::counts / units::us).isCountDensity());
  EXPECT_TRUE(Unit(units::counts / units::meV).isCountDensity());
  EXPECT_FALSE(Unit(units::dimensionless / units::m).isCountDensity());
}

TEST(UnitFunctionsTest, abs) {
  EXPECT_EQ(abs(units::one), units::one);
  EXPECT_EQ(abs(units::m), units::m);
}

TEST(UnitFunctionsTest, sqrt) {
  EXPECT_EQ(sqrt(units::m * units::m), units::m);
  EXPECT_EQ(sqrt(units::one), units::one);
  EXPECT_THROW_MSG(sqrt(units::m), except::UnitError,
                   "Unsupported unit as result of sqrt: sqrt(m).");
}

TEST(UnitFunctionsTest, sin) {
  EXPECT_EQ(sin(units::rad), units::dimensionless);
  EXPECT_EQ(sin(units::deg), units::dimensionless);
  EXPECT_THROW(sin(units::m), except::UnitError);
  EXPECT_THROW(sin(units::dimensionless), except::UnitError);
}

TEST(UnitFunctionsTest, cos) {
  EXPECT_EQ(cos(units::rad), units::dimensionless);
  EXPECT_EQ(cos(units::deg), units::dimensionless);
  EXPECT_THROW(cos(units::m), except::UnitError);
  EXPECT_THROW(cos(units::dimensionless), except::UnitError);
}

TEST(UnitFunctionsTest, tan) {
  EXPECT_EQ(tan(units::rad), units::dimensionless);
  EXPECT_EQ(tan(units::deg), units::dimensionless);
  EXPECT_THROW(tan(units::m), except::UnitError);
  EXPECT_THROW(tan(units::dimensionless), except::UnitError);
}

TEST(UnitFunctionsTest, asin) {
  EXPECT_EQ(asin(units::dimensionless), units::rad);
  EXPECT_THROW(asin(units::m), except::UnitError);
  EXPECT_THROW(asin(units::rad), except::UnitError);
  EXPECT_THROW(asin(units::deg), except::UnitError);
}

TEST(UnitFunctionsTest, acos) {
  EXPECT_EQ(acos(units::dimensionless), units::rad);
  EXPECT_THROW(acos(units::m), except::UnitError);
  EXPECT_THROW(acos(units::rad), except::UnitError);
  EXPECT_THROW(acos(units::deg), except::UnitError);
}

TEST(UnitFunctionsTest, atan) {
  EXPECT_EQ(atan(units::dimensionless), units::rad);
  EXPECT_THROW(atan(units::m), except::UnitError);
  EXPECT_THROW(atan(units::rad), except::UnitError);
  EXPECT_THROW(atan(units::deg), except::UnitError);
}

TEST(UnitFunctionsTest, atan2) {
  EXPECT_EQ(atan2(units::m, units::m), units::rad);
  EXPECT_EQ(atan2(units::s, units::s), units::rad);
  EXPECT_THROW(atan2(units::m, units::s), except::UnitError);
}
