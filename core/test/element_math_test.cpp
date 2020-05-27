// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/math.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

using namespace scipp;
using namespace scipp::core;

TEST(ElementAbsTest, unit) {
  units::Unit m(units::m);
  EXPECT_EQ(element::abs(m), units::abs(m));
}

TEST(ElementAbsTest, value) {
  EXPECT_EQ(element::abs(-1.23), std::abs(-1.23));
  EXPECT_EQ(element::abs(-1.23456789f), std::abs(-1.23456789f));
}

TEST(ElementAbsTest, value_and_variance) {
  const ValueAndVariance x(-2.0, 1.0);
  EXPECT_EQ(element::abs(x), abs(x));
}

TEST(ElementAbsOutArgTest, unit) {
  units::Unit m(units::m);
  units::Unit out(units::one);
  element::abs_out_arg(out, m);
  EXPECT_EQ(out, units::abs(m));
}

TEST(ElementAbsOutArgTest, value_double) {
  double out;
  element::abs_out_arg(out, -1.23);
  EXPECT_EQ(out, std::abs(-1.23));
}

TEST(ElementAbsOutArgTest, value_float) {
  float out;
  element::abs_out_arg(out, -1.23456789f);
  EXPECT_EQ(out, std::abs(-1.23456789f));
}

TEST(ElementAbsOutArgTest, value_and_variance) {
  const ValueAndVariance x(-2.0, 1.0);
  ValueAndVariance out(x);
  element::abs_out_arg(out, x);
  EXPECT_EQ(out, abs(x));
}

TEST(ElementAbsOutArgTest, supported_types) {
  auto supported = decltype(element::abs_out_arg)::types{};
  std::get<double>(supported);
  std::get<float>(supported);
}

TEST(ElementNormTest, unit) {
  const units::Unit s(units::s);
  const units::Unit m2(units::m * units::m);
  const units::Unit dimless(units::dimensionless);
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

TEST(ElementSqrtTest, unit) {
  const units::Unit m2(units::m * units::m);
  EXPECT_EQ(element::sqrt(m2), units::sqrt(m2));
}

TEST(ElementSqrtTest, value) {
  EXPECT_EQ(element::sqrt(1.23), std::sqrt(1.23));
  EXPECT_EQ(element::sqrt(1.23456789f), std::sqrt(1.23456789f));
}

TEST(ElementSqrtTest, value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  EXPECT_EQ(element::sqrt(x), sqrt(x));
}

TEST(ElementSqrtOutArgTest, unit) {
  const units::Unit m2(units::m * units::m);
  units::Unit out(units::one);
  element::sqrt_out_arg(out, m2);
  EXPECT_EQ(out, units::sqrt(m2));
}

TEST(ElementSqrtOutArgTest, value_double) {
  double out;
  element::sqrt_out_arg(out, 1.23);
  EXPECT_EQ(out, std::sqrt(1.23));
}

TEST(ElementSqrtOutArgTest, value_float) {
  float out;
  element::sqrt_out_arg(out, 1.23456789f);
  EXPECT_EQ(out, std::sqrt(1.23456789f));
}

TEST(ElementSqrtOutArgTest, value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  ValueAndVariance out(x);
  element::sqrt_out_arg(out, x);
  EXPECT_EQ(out, sqrt(x));
}

TEST(ElementSqrtOutArgTest, supported_types) {
  auto supported = decltype(element::sqrt_out_arg)::types{};
  std::get<double>(supported);
  std::get<float>(supported);
}

TEST(ElementDotTest, unit) {
  const units::Unit m(units::m);
  const units::Unit m2(units::m * units::m);
  const units::Unit dimless(units::dimensionless);
  EXPECT_EQ(element::dot(m, m), m2);
  EXPECT_EQ(element::dot(dimless, dimless), dimless);
}

TEST(ElementDotTest, value) {
  Eigen::Vector3d v1(0, 3, -4);
  Eigen::Vector3d v2(1, 1, -1);
  EXPECT_EQ(element::dot(v1, v1), 25);
  EXPECT_EQ(element::dot(v2, v2), 3);
}

TEST(ElementReciprocalTest, unit) {
  const units::Unit one_over_m(units::one / units::m);
  EXPECT_EQ(element::reciprocal(one_over_m), units::m);
  const units::Unit one_over_s(units::one / units::s);
  EXPECT_EQ(element::reciprocal(units::s), one_over_s);
}

TEST(ElementReciprocalTest, value) {
  EXPECT_EQ(element::reciprocal(1.23), 1 / 1.23);
  EXPECT_EQ(element::reciprocal(1.23456789f), 1 / 1.23456789f);
}

TEST(ElementReciprocalTest, value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  EXPECT_EQ(element::reciprocal(x), 1 / x);
}

TEST(ElementReciprocalOutArgTest, unit) {
  const units::Unit one_over_m(units::one / units::m);
  units::Unit out(units::one);
  element::reciprocal_out_arg(out, one_over_m);
  EXPECT_EQ(out, units::m);
  element::reciprocal_out_arg(out, units::s);
  const units::Unit one_over_s(units::one / units::s);
  EXPECT_EQ(out, one_over_s);
}

TEST(ElementReciprocalOutArgTest, value_double) {
  double out;
  element::reciprocal_out_arg(out, 1.23);
  EXPECT_EQ(out, 1 / 1.23);
}

TEST(ElementReciprocalOutArgTest, value_float) {
  float out;
  element::reciprocal_out_arg(out, 1.23456789f);
  EXPECT_EQ(out, 1 / 1.23456789f);
}

TEST(ElementReciprocalOutArgTest, value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  ValueAndVariance out(x);
  element::reciprocal_out_arg(out, x);
  EXPECT_EQ(out, 1 / x);
}

TEST(ElementReciprocalOutArgTest, supported_types) {
  auto supported = decltype(element::reciprocal_out_arg)::types{};
  std::get<double>(supported);
  std::get<float>(supported);
}
