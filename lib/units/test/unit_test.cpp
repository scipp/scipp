// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/units/except.h"
#include "scipp/units/unit.h"

using namespace scipp;
using scipp::sc_units::Unit;

TEST(UnitTest, c) {
  EXPECT_EQ(sc_units::c.underlying().multiplier(), 299792458.0);
}

TEST(UnitTest, cancellation) {
  EXPECT_EQ(Unit(sc_units::deg / sc_units::deg), sc_units::dimensionless);
  EXPECT_EQ(sc_units::deg / sc_units::deg, sc_units::dimensionless);
  EXPECT_EQ(sc_units::deg * Unit(sc_units::rad / sc_units::deg), sc_units::rad);
}

TEST(UnitTest, construct) { ASSERT_NO_THROW(Unit{sc_units::dimensionless}); }

TEST(UnitTest, construct_default) {
  Unit u;
  ASSERT_EQ(u, sc_units::none);
}

TEST(UnitTest, construct_bad_string) {
  EXPECT_THROW(Unit("abcde"), except::UnitError);
}

TEST(UnitTest, custom_unit_strings_get_rejected) {
  // Custom (counting) units and equation units are rejected.
  for (const auto &str : {"CXUN[0]", "CXUN[51]", "CXUN[1023]", "CXCUN[0]",
                          "CXCUN[15]", "{corn}", "{CXCOMM[105]}"}) {
    EXPECT_THROW_DISCARD(Unit(str), except::UnitError);
  }
}

TEST(UnitTest, overflows) {
  // These would run out of bits in llnl/units and wrap, ensure that scipp
  // prevents this and throws instead.
  Unit m64{pow(sc_units::m, 64)};
  Unit inv_m128{sc_units::one / m64 / m64};
  EXPECT_THROW(m64 * m64, except::UnitError);
  EXPECT_THROW(sc_units::one / inv_m128, except::UnitError);
  EXPECT_THROW(inv_m128 / sc_units::m, except::UnitError);
  EXPECT_THROW(pow(sc_units::m, 128), except::UnitError);
}

TEST(UnitTest, compare) {
  Unit u1{sc_units::dimensionless};
  Unit u2{sc_units::m};
  ASSERT_TRUE(u1 == u1);
  ASSERT_TRUE(u1 != u2);
  ASSERT_TRUE(u2 == u2);
  ASSERT_FALSE(u1 == u2);
  ASSERT_FALSE(u2 != u2);
}

TEST(UnitTest, add) {
  Unit a{sc_units::dimensionless};
  Unit b{sc_units::m};
  Unit c{sc_units::m * sc_units::m};
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
  Unit a{sc_units::dimensionless};
  Unit b{sc_units::m};
  Unit c{sc_units::m * sc_units::m};
  EXPECT_EQ(a * a, a);
  EXPECT_EQ(a * b, b);
  EXPECT_EQ(b * a, b);
  EXPECT_EQ(a * c, c);
  EXPECT_EQ(c * a, c);
  EXPECT_EQ(b * b, c);
  EXPECT_EQ(b * c, sc_units::m * sc_units::m * sc_units::m);
  EXPECT_EQ(c * b, sc_units::m * sc_units::m * sc_units::m);
}

TEST(UnitTest, counts_variances) {
  Unit counts{sc_units::counts};
  EXPECT_EQ(counts * counts, sc_units::Unit("counts**2"));
}

TEST(UnitTest, multiply_counts) {
  Unit counts{sc_units::counts};
  Unit none{sc_units::dimensionless};
  EXPECT_EQ(counts * none, counts);
  EXPECT_EQ(none * counts, counts);
}

TEST(UnitTest, divide) {
  Unit one{sc_units::dimensionless};
  Unit l{sc_units::m};
  Unit t{sc_units::s};
  Unit v{sc_units::m / sc_units::s};
  EXPECT_EQ(l / one, l);
  EXPECT_EQ(t / one, t);
  EXPECT_EQ(l / l, one);
  EXPECT_EQ(l / t, v);
}

TEST(UnitTest, divide_counts) {
  Unit counts{sc_units::counts};
  EXPECT_EQ(counts / counts, sc_units::dimensionless);
}

TEST(UnitTest, modulo) {
  Unit one{sc_units::dimensionless};
  Unit l{sc_units::m};
  Unit t{sc_units::s};
  Unit none{sc_units::none};
  EXPECT_EQ(l % l, l);
  EXPECT_EQ(t % t, t);
  EXPECT_THROW(l % t, except::UnitError);
  EXPECT_THROW(l % one, except::UnitError);
  EXPECT_THROW(l % none, except::UnitError);
  EXPECT_THROW(t % l, except::UnitError);
}

TEST(UnitTest, pow) {
  EXPECT_EQ(pow(sc_units::m, 0), sc_units::one);
  EXPECT_EQ(pow(sc_units::m, 1), sc_units::m);
  EXPECT_EQ(pow(sc_units::m, 2), sc_units::m * sc_units::m);
  EXPECT_EQ(pow(sc_units::m, -1), sc_units::one / sc_units::m);
}

TEST(UnitTest, neutron_units) {
  Unit c(sc_units::c);
  EXPECT_EQ(c * sc_units::m, Unit(sc_units::c * sc_units::m));
  EXPECT_EQ(c * sc_units::m / sc_units::m, sc_units::c);
  EXPECT_EQ(sc_units::meV / c, Unit(sc_units::meV / sc_units::c));
  EXPECT_EQ(sc_units::meV / c / sc_units::meV,
            Unit(sc_units::dimensionless / sc_units::c));
}

TEST(UnitTest, isCounts) {
  EXPECT_FALSE(sc_units::dimensionless.isCounts());
  EXPECT_TRUE(sc_units::counts.isCounts());
  EXPECT_FALSE(Unit(sc_units::counts / sc_units::us).isCounts());
  EXPECT_FALSE(Unit(sc_units::counts / sc_units::meV).isCounts());
  EXPECT_FALSE(Unit(sc_units::dimensionless / sc_units::m).isCounts());
}

TEST(UnitTest, isCountDensity) {
  EXPECT_FALSE(sc_units::dimensionless.isCountDensity());
  EXPECT_FALSE(sc_units::counts.isCountDensity());
  EXPECT_TRUE(Unit(sc_units::counts / sc_units::us).isCountDensity());
  EXPECT_TRUE(Unit(sc_units::counts / sc_units::meV).isCountDensity());
  EXPECT_FALSE(Unit(sc_units::dimensionless / sc_units::m).isCountDensity());
}

TEST(UnitFunctionsTest, abs) {
  EXPECT_EQ(abs(sc_units::one), sc_units::one);
  EXPECT_EQ(abs(sc_units::m), sc_units::m);
}

TEST(UnitFunctionsTest, ceil) {
  EXPECT_EQ(ceil(sc_units::one), sc_units::one);
  EXPECT_EQ(ceil(sc_units::m), sc_units::m);
}

TEST(UnitFunctionsTest, floor) {
  EXPECT_EQ(floor(sc_units::one), sc_units::one);
  EXPECT_EQ(floor(sc_units::m), sc_units::m);
}

TEST(UnitFunctionsTest, rint) {
  EXPECT_EQ(rint(sc_units::one), sc_units::one);
  EXPECT_EQ(rint(sc_units::m), sc_units::m);
}

TEST(UnitFunctionsTest, sqrt) {
  EXPECT_EQ(sqrt(sc_units::m * sc_units::m), sc_units::m);
  EXPECT_EQ(sqrt(sc_units::counts * sc_units::counts), sc_units::counts);
  EXPECT_EQ(sqrt(sc_units::one), sc_units::one);
  EXPECT_THROW_MSG(sqrt(sc_units::m), except::UnitError,
                   "Unsupported unit as result of sqrt: sqrt(m).");
  EXPECT_THROW_MSG(sqrt(sc_units::Unit("J")), except::UnitError,
                   "Unsupported unit as result of sqrt: sqrt(J).");
  EXPECT_THROW_MSG(sqrt(sc_units::Unit("eV")), except::UnitError,
                   "Unsupported unit as result of sqrt: sqrt(eV).");
}

TEST(UnitFunctionsTest, sin) {
  EXPECT_EQ(sin(sc_units::rad), sc_units::dimensionless);
  EXPECT_EQ(sin(sc_units::deg), sc_units::dimensionless);
  EXPECT_THROW(sin(sc_units::m), except::UnitError);
  EXPECT_THROW(sin(sc_units::dimensionless), except::UnitError);
}

TEST(UnitFunctionsTest, cos) {
  EXPECT_EQ(cos(sc_units::rad), sc_units::dimensionless);
  EXPECT_EQ(cos(sc_units::deg), sc_units::dimensionless);
  EXPECT_THROW(cos(sc_units::m), except::UnitError);
  EXPECT_THROW(cos(sc_units::dimensionless), except::UnitError);
}

TEST(UnitFunctionsTest, tan) {
  EXPECT_EQ(tan(sc_units::rad), sc_units::dimensionless);
  EXPECT_EQ(tan(sc_units::deg), sc_units::dimensionless);
  EXPECT_THROW(tan(sc_units::m), except::UnitError);
  EXPECT_THROW(tan(sc_units::dimensionless), except::UnitError);
}

TEST(UnitFunctionsTest, asin) {
  EXPECT_EQ(asin(sc_units::dimensionless), sc_units::rad);
  EXPECT_THROW(asin(sc_units::m), except::UnitError);
  EXPECT_THROW(asin(sc_units::rad), except::UnitError);
  EXPECT_THROW(asin(sc_units::deg), except::UnitError);
}

TEST(UnitFunctionsTest, acos) {
  EXPECT_EQ(acos(sc_units::dimensionless), sc_units::rad);
  EXPECT_THROW(acos(sc_units::m), except::UnitError);
  EXPECT_THROW(acos(sc_units::rad), except::UnitError);
  EXPECT_THROW(acos(sc_units::deg), except::UnitError);
}

TEST(UnitFunctionsTest, atan) {
  EXPECT_EQ(atan(sc_units::dimensionless), sc_units::rad);
  EXPECT_THROW(atan(sc_units::m), except::UnitError);
  EXPECT_THROW(atan(sc_units::rad), except::UnitError);
  EXPECT_THROW(atan(sc_units::deg), except::UnitError);
}

TEST(UnitFunctionsTest, atan2) {
  EXPECT_EQ(atan2(sc_units::m, sc_units::m), sc_units::rad);
  EXPECT_EQ(atan2(sc_units::s, sc_units::s), sc_units::rad);
  EXPECT_THROW(atan2(sc_units::m, sc_units::s), except::UnitError);
}

TEST(UnitFunctionsTest, sinh) {
  EXPECT_EQ(sinh(sc_units::dimensionless), sc_units::dimensionless);
  EXPECT_THROW(sinh(sc_units::m), except::UnitError);
}

TEST(UnitFunctionsTest, cosh) {
  EXPECT_EQ(cosh(sc_units::dimensionless), sc_units::dimensionless);
  EXPECT_THROW(cosh(sc_units::m), except::UnitError);
}

TEST(UnitFunctionsTest, tanh) {
  EXPECT_EQ(tanh(sc_units::dimensionless), sc_units::dimensionless);
  EXPECT_THROW(tanh(sc_units::m), except::UnitError);
}

TEST(UnitFunctionsTest, asinh) {
  EXPECT_EQ(asinh(sc_units::dimensionless), sc_units::dimensionless);
  EXPECT_THROW(asinh(sc_units::m), except::UnitError);
}

TEST(UnitFunctionsTest, acosh) {
  EXPECT_EQ(acosh(sc_units::dimensionless), sc_units::dimensionless);
  EXPECT_THROW(acosh(sc_units::m), except::UnitError);
}

TEST(UnitFunctionsTest, atanh) {
  EXPECT_EQ(atanh(sc_units::dimensionless), sc_units::dimensionless);
  EXPECT_THROW(atanh(sc_units::m), except::UnitError);
}

TEST(UnitParseTest, singular_plural) {
  EXPECT_EQ(sc_units::Unit("counts"), sc_units::counts);
  EXPECT_EQ(sc_units::Unit("count"), sc_units::counts);
}

TEST(UnitFormatTest, roundtrip_string) {
  for (const auto &s :
       {"m",        "m/s",       "meV",      "pAh",        "mAh",
        "ns",       "counts",    "counts^2", "counts/meV", "1/counts",
        "counts/m", "rad",       "$",        "Y",          "M",
        "D",        "arb. unit", "EQXUN[1]", "EQXUN[23]",  "°C"}) {
    const auto unit = sc_units::Unit(s);
    EXPECT_EQ(to_string(unit), s);
    EXPECT_EQ(sc_units::Unit(to_string(unit)), unit);
  }
}

TEST(UnitFormatTest, roundtrip_unit) {
  // Some strings use special characters, e.g., for micro and Angstrom, but
  // some are actually formatted badly right now, but at least roundtrip works.
  for (const auto &s : {"us", "angstrom", "counts/us", "Y", "M", "D",
                        "decibels", "a.u.", "arbitraryunit", "Sv", "degC"}) {
    const auto unit = sc_units::Unit(s);
    EXPECT_EQ(sc_units::Unit(to_string(unit)), unit);
  }
}

TEST(UnitTest, binary_operations_with_one_none_operand_throw_UnitError) {
  using sc_units::none;
  const auto u = sc_units::m;
  EXPECT_THROW_DISCARD(none + u, except::UnitError);
  EXPECT_THROW_DISCARD(u + none, except::UnitError);
  EXPECT_THROW_DISCARD(none - u, except::UnitError);
  EXPECT_THROW_DISCARD(u - none, except::UnitError);
  EXPECT_THROW_DISCARD(none * u, except::UnitError);
  EXPECT_THROW_DISCARD(u * none, except::UnitError);
  EXPECT_THROW_DISCARD(none / u, except::UnitError);
  EXPECT_THROW_DISCARD(u / none, except::UnitError);
  EXPECT_THROW_DISCARD(none % u, except::UnitError);
  EXPECT_THROW_DISCARD(u % none, except::UnitError);
  EXPECT_THROW_DISCARD(atan2(u, none), except::UnitError);
  EXPECT_THROW_DISCARD(atan2(none, u), except::UnitError);
}

TEST(UnitTest,
     inplace_binary_operations_with_one_none_operand_throw_UnitError) {
  auto none = sc_units::none;
  auto u = sc_units::m;
  EXPECT_THROW_DISCARD(none += u, except::UnitError);
  EXPECT_THROW_DISCARD(u += none, except::UnitError);
  EXPECT_THROW_DISCARD(none -= u, except::UnitError);
  EXPECT_THROW_DISCARD(u -= none, except::UnitError);
  EXPECT_THROW_DISCARD(none *= u, except::UnitError);
  EXPECT_THROW_DISCARD(u *= none, except::UnitError);
  EXPECT_THROW_DISCARD(none /= u, except::UnitError);
  EXPECT_THROW_DISCARD(u /= none, except::UnitError);
  EXPECT_THROW_DISCARD(none %= u, except::UnitError);
  EXPECT_THROW_DISCARD(u %= none, except::UnitError);
}

TEST(UnitTest, binary_operations_with_two_none_operands_return_none) {
  using sc_units::none;
  EXPECT_EQ(none + none, none);
  EXPECT_EQ(none - none, none);
  EXPECT_EQ(none * none, none);
  EXPECT_EQ(none / none, none); // cppcheck-suppress duplicateExpression
  EXPECT_EQ(none % none, none); // cppcheck-suppress duplicateExpression
}

TEST(UnitTest, trigonometric_of_none_throw_UnitError) {
  using sc_units::none;
  EXPECT_THROW_DISCARD(sin(none), except::UnitError);
  EXPECT_THROW_DISCARD(cos(none), except::UnitError);
  EXPECT_THROW_DISCARD(tan(none), except::UnitError);
}

TEST(UnitTest, inverse_trigonometric_of_none_throw_UnitError) {
  using sc_units::none;
  EXPECT_THROW_DISCARD(asin(none), except::UnitError);
  EXPECT_THROW_DISCARD(acos(none), except::UnitError);
  EXPECT_THROW_DISCARD(atan(none), except::UnitError);
  EXPECT_THROW_DISCARD(atan2(none, none), except::UnitError);
}

TEST(UnitTest, sqrt_of_none_returns_none) {
  EXPECT_EQ(sqrt(sc_units::none), sc_units::none);
}

TEST(UnitTest, pow_of_none_returns_none) {
  EXPECT_EQ(sqrt(sc_units::none), sc_units::none);
}
