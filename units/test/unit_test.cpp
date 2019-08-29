// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/units/except.h"
#include "scipp/units/unit.h"

using namespace scipp;
using scipp::units::Unit;

TEST(DummyUnitsTest, basics) {
  // Current neutron::Unit is inlined as Unit, but we can still use others.
  units::dummy::Unit m{units::m};
  units::dummy::Unit s{units::s};
  ASSERT_NE(m, s);
  units::dummy::Unit expected{units::m / units::s};
  auto result = m / s;
  EXPECT_EQ(result, expected);
}

TEST(units, c) {
  auto c = 1.0 * units::c;
  EXPECT_EQ(c.value(), 1.0);

  boost::units::quantity<boost::units::si::velocity> si_c(c);
  EXPECT_EQ(si_c.value(), 299792458.0);
}

TEST(Units, sin) {
  EXPECT_EQ(sin(Unit(units::rad)), units::dimensionless);
  EXPECT_EQ(sin(Unit(units::deg)), units::dimensionless);
  EXPECT_THROW(sin(Unit(units::m)), except::UnitError);
  EXPECT_THROW(sin(Unit(units::dimensionless)), except::UnitError);
}

TEST(Units, cos) {
  EXPECT_EQ(cos(Unit(units::rad)), units::dimensionless);
  EXPECT_EQ(cos(Unit(units::deg)), units::dimensionless);
  EXPECT_THROW(cos(Unit(units::m)), except::UnitError);
  EXPECT_THROW(cos(Unit(units::dimensionless)), except::UnitError);
}

TEST(Units, tan) {
  EXPECT_EQ(tan(Unit(units::rad)), units::dimensionless);
  EXPECT_EQ(tan(Unit(units::deg)), units::dimensionless);
  EXPECT_THROW(tan(Unit(units::m)), except::UnitError);
  EXPECT_THROW(tan(Unit(units::dimensionless)), except::UnitError);
}

TEST(Units, asin) {
  EXPECT_EQ(asin(Unit(units::dimensionless)), units::rad);
  EXPECT_THROW(asin(Unit(units::m)), except::UnitError);
  EXPECT_THROW(asin(Unit(units::rad)), except::UnitError);
  EXPECT_THROW(asin(Unit(units::deg)), except::UnitError);
}

TEST(Units, acos) {
  EXPECT_EQ(acos(Unit(units::dimensionless)), units::rad);
  EXPECT_THROW(acos(Unit(units::m)), except::UnitError);
  EXPECT_THROW(acos(Unit(units::rad)), except::UnitError);
  EXPECT_THROW(acos(Unit(units::deg)), except::UnitError);
}

TEST(Units, atan) {
  EXPECT_EQ(atan(Unit(units::dimensionless)), units::rad);
  EXPECT_THROW(atan(Unit(units::m)), except::UnitError);
  EXPECT_THROW(atan(Unit(units::rad)), except::UnitError);
  EXPECT_THROW(atan(Unit(units::deg)), except::UnitError);
}

TEST(Unit, construct) { ASSERT_NO_THROW(Unit u{units::dimensionless}); }

TEST(Unit, construct_default) {
  Unit u;
  ASSERT_EQ(u, units::dimensionless);
}

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
  EXPECT_THROW(a + b, except::UnitError);
  EXPECT_THROW(a + c, except::UnitError);
  EXPECT_THROW(b + a, except::UnitError);
  EXPECT_THROW(b + c, except::UnitError);
  EXPECT_THROW(c + a, except::UnitError);
  EXPECT_THROW(c + b, except::UnitError);
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
  EXPECT_EQ(b * c, units::m * units::m * units::m);
  EXPECT_EQ(c * b, units::m * units::m * units::m);
  EXPECT_THROW(c * c, except::UnitError);
}

TEST(Unit, multiply_counts) {
  Unit counts{units::counts};
  Unit none{units::dimensionless};
  EXPECT_EQ(counts * none, counts);
  EXPECT_EQ(none * counts, counts);
}

TEST(Unit, divide) {
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

TEST(Unit, conversion_factors) {
  boost::units::quantity<units::detail::tof::wavelength> a(2.0 *
                                                           units::angstrom);
  boost::units::quantity<boost::units::si::length> b(3.0 * units::angstrom);
  boost::units::quantity<units::detail::tof::wavelength> c(
      4.0 * boost::units::si::meters);
  boost::units::quantity<boost::units::si::area> d(
      5.0 * boost::units::si::meters * units::angstrom);
  boost::units::quantity<units::detail::tof::energy> e = 6.0 * units::meV;
  boost::units::quantity<boost::units::si::energy> f(7.0 * units::meV);
  boost::units::quantity<boost::units::si::time> g(8.0 * units::us);
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
  EXPECT_THROW_MSG(sqrt(m), except::UnitError,
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
  EXPECT_TRUE(units::dummy::Unit(units::dimensionless).isCounts());
  EXPECT_FALSE(units::dummy::Unit(units::dimensionless / units::m).isCounts());
}

TEST(DummyUnitsTest, isCountDensity) {
  EXPECT_FALSE(units::dummy::Unit(units::dimensionless).isCountDensity());
  EXPECT_TRUE(
      units::dummy::Unit(units::dimensionless / units::m).isCountDensity());
}
