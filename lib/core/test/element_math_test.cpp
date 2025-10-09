// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/math.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::core;

TEST(ElementAbsTest, unit) {
  sc_units::Unit m(sc_units::m);
  EXPECT_EQ(element::abs(m), sc_units::abs(m));
}

TEST(ElementAbsTest, value) {
  EXPECT_EQ(element::abs(-1.23), std::abs(-1.23));
  EXPECT_EQ(element::abs(-1.23456789f), std::abs(-1.23456789f));
}

TEST(ElementAbsTest, value_and_variance) {
  const ValueAndVariance x(-2.0, 1.0);
  EXPECT_EQ(element::abs(x), abs(x));
}

TEST(ElementAbsTest, supported_types) {
  auto supported = decltype(element::abs)::types{};
  static_cast<void>(std::get<double>(supported));
  static_cast<void>(std::get<float>(supported));
}

TEST(ElementNormTest, unit) {
  const sc_units::Unit s(sc_units::s);
  const sc_units::Unit m2(sc_units::m * sc_units::m);
  const sc_units::Unit dimless(sc_units::dimensionless);
  EXPECT_EQ(element::norm(m2), m2);
  EXPECT_EQ(element::norm(s), s);
  EXPECT_EQ(element::norm(dimless), dimless);
}

TEST(ElementNormTest, value) {
  Eigen::Vector3d v1(0, 3, 4);
  Eigen::Vector3d v2(3, 0, -4);
  EXPECT_EQ(element::norm(v1), 5);
  EXPECT_EQ(element::norm(v2), 5);
}

TEST(ElementPowTest, unit) {
  // element::pow cannot handle units itself as that requires the *value* of the
  // exponent and not its unit. This does not fit into the usual transform
  // framework.
  EXPECT_EQ(element::pow(sc_units::one, sc_units::one), sc_units::one);
  EXPECT_THROW_DISCARD(element::pow(sc_units::one, sc_units::m),
                       except::UnitError);
  EXPECT_THROW_DISCARD(element::pow(sc_units::s, sc_units::one),
                       except::UnitError);
  EXPECT_THROW_DISCARD(element::pow(sc_units::K, sc_units::kg),
                       except::UnitError);
}

TEST(ElementPowTest, value) {
  EXPECT_NEAR(element::pow(3.0, 2.0), 9.0, 1e-15);
  EXPECT_NEAR(element::pow(int64_t{3}, 2.0), 9.0, 1e-15);
  EXPECT_NEAR(element::pow(3.0, int64_t{2}), 9.0, 1e-15);
  EXPECT_EQ(element::pow(int64_t{3}, int64_t{2}), 9);
}

TEST(ElementPowTest, value_and_variance) {
  const ValueAndVariance base{3.0, 2.0};
  EXPECT_EQ(element::pow(base, int64_t{3}), pow(base, int64_t{3}));
}

TEST(ElementSqrtTest, unit) {
  const sc_units::Unit m2(sc_units::m * sc_units::m);
  EXPECT_EQ(element::sqrt(m2), sc_units::sqrt(m2));
}

TEST(ElementSqrtTest, value) {
  EXPECT_EQ(element::sqrt(1.23), std::sqrt(1.23));
  EXPECT_EQ(element::sqrt(1.23456789f), std::sqrt(1.23456789f));
}

TEST(ElementSqrtTest, value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  EXPECT_EQ(element::sqrt(x), sqrt(x));
}

TEST(ElementSqrtTest, supported_types) {
  auto supported = decltype(element::sqrt)::types{};
  static_cast<void>(std::get<double>(supported));
  static_cast<void>(std::get<float>(supported));
}

template <class T> void element_vector_op_units_test(T op) {
  const sc_units::Unit m(sc_units::m);
  const sc_units::Unit m2(sc_units::m * sc_units::m);
  const sc_units::Unit dimless(sc_units::dimensionless);
  EXPECT_EQ(op(m, m), m2);
  EXPECT_EQ(op(dimless, dimless), dimless);
}

TEST(ElementDotTest, unit) { element_vector_op_units_test(element::dot); }

TEST(ElementDotTest, value) {
  Eigen::Vector3d v1(0, 0, 1);
  Eigen::Vector3d v2(1, 0, 0);
  EXPECT_EQ(element::cross(v1, v2), Eigen::Vector3d(0, 1, 0));
  EXPECT_EQ(element::cross(v2, v1), Eigen::Vector3d(0, -1, 0));
  EXPECT_EQ(element::cross(v2, v2), Eigen::Vector3d(0, 0, 0));
}

TEST(ElementCrossTest, unit) { element_vector_op_units_test(element::cross); }

TEST(ElementCrossTest, value) {
  Eigen::Vector3d v1(0, 3, -4);
  Eigen::Vector3d v2(1, 1, -1);
  EXPECT_EQ(element::dot(v1, v1), 25);
  EXPECT_EQ(element::dot(v2, v2), 3);
}

TEST(ElementReciprocalTest, unit) {
  const sc_units::Unit one_over_m(sc_units::one / sc_units::m);
  EXPECT_EQ(element::reciprocal(one_over_m), sc_units::m);
  const sc_units::Unit one_over_s(sc_units::one / sc_units::s);
  EXPECT_EQ(element::reciprocal(sc_units::s), one_over_s);
}

TEST(ElementReciprocalTest, value) {
  EXPECT_EQ(element::reciprocal(1.23), 1 / 1.23);
  EXPECT_EQ(element::reciprocal(1.23456789f), 1 / 1.23456789f);
}

TEST(ElementReciprocalTest, value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  EXPECT_EQ(element::reciprocal(x), 1 / x);
}

TEST(ElementExpTest, value) {
  EXPECT_EQ(element::exp(1.23), std::exp(1.23));
  EXPECT_EQ(element::exp(1.23456789f), std::exp(1.23456789f));
}

TEST(ElementExpTest, unit) {
  EXPECT_EQ(element::exp(sc_units::dimensionless), sc_units::dimensionless);
}

TEST(ElementExpTest, bad_unit) { EXPECT_ANY_THROW(element::exp(sc_units::m)); }

TEST(ElementLogTest, value) {
  EXPECT_EQ(element::log(1.23), std::log(1.23));
  EXPECT_EQ(element::log(1.23456789f), std::log(1.23456789f));
}

TEST(ElementLogTest, unit) {
  EXPECT_EQ(element::log(sc_units::dimensionless), sc_units::dimensionless);
}

TEST(ElementLogTest, bad_unit) { EXPECT_ANY_THROW(element::log(sc_units::m)); }

TEST(ElementLog10Test, value) {
  EXPECT_EQ(element::log10(1.23), std::log10(1.23));
  EXPECT_EQ(element::log10(1.23456789f), std::log10(1.23456789f));
}

TEST(ElementLog10Test, unit) {
  EXPECT_EQ(element::log10(sc_units::dimensionless), sc_units::dimensionless);
}

TEST(ElementLog10Test, bad_unit) {
  EXPECT_ANY_THROW(element::log10(sc_units::m));
}

namespace {
template <class T>
void elementRoundingTest(T rounding_function, const std::vector<float> &input,
                         const std::vector<float> &output) {
  for (auto i = 0u; i < input.size(); i++) {
    EXPECT_EQ(rounding_function(input[i]), output[i]);
  }
}
} // namespace

TEST(ElementRoundingTest, floor) {
  elementRoundingTest(element::floor, {2.5, 2.7, 2.3, 2.15, 2.617, 2.32133},
                      {2., 2., 2., 2., 2., 2.});
}

TEST(ElementRoundingTest, ceil) {
  elementRoundingTest(element::ceil, {2.5, 2.7, 2.3, 2.15, 2.617, 2.32133},
                      {3., 3., 3., 3., 3., 3.});
}

TEST(ElementRoundingTest, rint) {
  elementRoundingTest(
      element::rint, {2.01, 2.7, 2.3, 2.15, 2.617, 2.32133, 1.5, 2.5, 3.5, 4.5},
      {2., 3., 2., 2., 3., 2., 2., 2., 4., 4.});
}

TEST(ElementMathTest, erf) {
  EXPECT_EQ(element::erf(1.1), std::erf(1.1));
  EXPECT_EQ(element::erf(sc_units::one), sc_units::one);
  EXPECT_THROW_DISCARD(element::erf(sc_units::m), except::UnitError);
}

TEST(ElementMathTest, erfc) {
  EXPECT_EQ(element::erfc(1.1), std::erfc(1.1));
  EXPECT_EQ(element::erfc(sc_units::one), sc_units::one);
  EXPECT_THROW_DISCARD(element::erfc(sc_units::m), except::UnitError);
}

TEST(ElementMathTest, midpoint_of_medium_sized_int) {
  EXPECT_EQ(element::midpoint(5, 1), 3);
  EXPECT_EQ(element::midpoint(2, 8), 5);
  EXPECT_EQ(element::midpoint(-4, -8), -6);
  EXPECT_EQ(element::midpoint(6, -8), -1);
}

TEST(ElementMathTest, midpoint_of_medium_sized_double) {
  EXPECT_EQ(element::midpoint(5.0, 1.0), 3.0);
  EXPECT_EQ(element::midpoint(2.0, 8.0), 5.0);
  EXPECT_EQ(element::midpoint(-4.0, -8.0), -6.0);
  EXPECT_EQ(element::midpoint(6.0, -7.0), -0.5);
}

TEST(ElementMathTest,
     midpoint_of_medium_sized_values_does_not_round_towards_first) {
  EXPECT_EQ(element::midpoint(4, 1), 2.5);
  EXPECT_EQ(element::midpoint(2, 7), 4.5);
  EXPECT_EQ(element::midpoint(-3, -8), -5.5);
  EXPECT_EQ(element::midpoint(6, -7), -0.5);
}

TEST(ElementMathTest, midpoint_of_small_int_works) {
  const auto a = std::numeric_limits<std::uint32_t>::max();
  const auto b = std::numeric_limits<std::uint32_t>::max() - 2;
  EXPECT_EQ(element::midpoint(a, b),
            std::numeric_limits<std::uint32_t>::max() - 1);
}

TEST(ElementMathTest, midpoint_of_time_point_uses_underlying_int) {
  const time_point a{123};
  const time_point b{732};
  EXPECT_EQ(element::midpoint(a, b), time_point{std::midpoint(123, 732)});
}

TEST(ElementMathTest, midpoint_units_must_be_equal) {
  EXPECT_EQ(element::midpoint(sc_units::m, sc_units::m), sc_units::m);
  EXPECT_THROW_DISCARD(element::midpoint(sc_units::m, sc_units::one),
                       except::UnitError);
}
