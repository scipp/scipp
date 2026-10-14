// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
#include <numbers>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "scipp/core/element/vector.h"
#include "scipp/units/unit.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::core;

TEST(ElementNormTest, unit) {
  const sc_units::Unit s(sc_units::s);
  const sc_units::Unit m2(sc_units::m * sc_units::m);
  const sc_units::Unit dimless(sc_units::dimensionless);
  EXPECT_EQ(element::norm(m2), m2);
  EXPECT_EQ(element::norm(s), s);
  EXPECT_EQ(element::norm(dimless), dimless);
}

TEST(ElementNormTest, value) {
  const Eigen::Vector3d v1(0, 3, 4);
  const Eigen::Vector3d v2(3, 0, -4);
  EXPECT_EQ(element::norm(v1), 5);
  EXPECT_EQ(element::norm(v2), 5);
}

namespace {
template <class T> void element_vector_binary_units_test(T op) {
  const sc_units::Unit m(sc_units::m);
  const sc_units::Unit m2(sc_units::m * sc_units::m);
  const sc_units::Unit dimless(sc_units::dimensionless);
  EXPECT_EQ(op(m, m), m2);
  EXPECT_EQ(op(dimless, dimless), dimless);
}
} // namespace

TEST(ElementDotTest, unit) { element_vector_binary_units_test(element::dot); }

TEST(ElementDotTest, value) {
  const Eigen::Vector3d v1(0, 0, 1);
  const Eigen::Vector3d v2(1, 0, 0);
  EXPECT_EQ(element::cross(v1, v2), Eigen::Vector3d(0, 1, 0));
  EXPECT_EQ(element::cross(v2, v1), Eigen::Vector3d(0, -1, 0));
  EXPECT_EQ(element::cross(v2, v2), Eigen::Vector3d(0, 0, 0));
}

TEST(ElementCrossTest, unit) {
  element_vector_binary_units_test(element::cross);
}

TEST(ElementCrossTest, value) {
  const Eigen::Vector3d v1(0, 3, -4);
  const Eigen::Vector3d v2(1, 1, -1);
  EXPECT_EQ(element::dot(v1, v1), 25);
  EXPECT_EQ(element::dot(v2, v2), 3);
  EXPECT_EQ(element::dot(v1, v2), 7);
}

TEST(ElementAngleBetweenTest, unit) {
  const sc_units::Unit s(sc_units::s);
  const sc_units::Unit m2(sc_units::m * sc_units::m);
  const sc_units::Unit dimless(sc_units::dimensionless);
  const sc_units::Unit rad(sc_units::rad);
  EXPECT_EQ(element::angle_between(s, s), rad);
  EXPECT_EQ(element::angle_between(s, m2), rad);
  EXPECT_EQ(element::angle_between(dimless, dimless), rad);
}

TEST(ElementAngleBetweenTest, value_expected) {
  using std::numbers::pi;

  const Eigen::Vector3d v1(1.5, 0, 0);
  const Eigen::Vector3d v2(1.3, 1.3, 0);
  const Eigen::Vector3d v3(0.0, -7.5, 0);
  const Eigen::Vector3d v4(-1.3, 1.3, 0);

  EXPECT_EQ(element::angle_between(v1, v1), 0);
  EXPECT_EQ(element::angle_between(v2, v2), 0);

  EXPECT_EQ(element::angle_between(v1, -v1), pi);
  EXPECT_EQ(element::angle_between(-v2, v2), pi);

  EXPECT_EQ(element::angle_between(v1, v2), pi / 4);
  EXPECT_EQ(element::angle_between(v2, v1), pi / 4);

  EXPECT_EQ(element::angle_between(v1, v3), pi / 2);
  EXPECT_EQ(element::angle_between(v3, v1), pi / 2);

  EXPECT_EQ(element::angle_between(v1, v4), 3 * pi / 4);
  EXPECT_EQ(element::angle_between(v4, v1), 3 * pi / 4);
}

TEST(ElementAngleBetweenTest, value_property) {
  const Eigen::Vector3d v1(-0.3, 4.2, 2.9);
  const Eigen::Vector3d v2(1.3, 0, -0.7);
  const auto computed =
      v1.norm() * v2.norm() * cos(element::angle_between(v1, v2));
  const auto expected = v1.dot(v2);
  EXPECT_THAT(computed, ::testing::NanSensitiveDoubleNear(expected, 1e-5f));
}
