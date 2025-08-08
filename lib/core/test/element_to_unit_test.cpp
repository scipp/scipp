// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <limits>

#include "scipp/core/element/to_unit.h"

using namespace scipp;
using namespace scipp::core;
using scipp::core::element::to_unit;

TEST(ElementToUnitTest, unit) {
  // Unit is simply replaced, not multiplied
  EXPECT_EQ(to_unit(sc_units::s, sc_units::us), sc_units::us);
}

TEST(ElementToUnitTest, type_preserved) {
  static_assert(std::is_same_v<decltype(to_unit(double(1), 1.0)), double>);
  static_assert(std::is_same_v<decltype(to_unit(float(1), 1.0)), float>);
  static_assert(std::is_same_v<decltype(to_unit(int64_t(1), 1.0)), int64_t>);
  static_assert(std::is_same_v<decltype(to_unit(int32_t(1), 1.0)), int32_t>);
  static_assert(std::is_same_v<decltype(to_unit(core::time_point(1), 1.0)),
                               core::time_point>);
  static_assert(std::is_same_v<decltype(to_unit(Eigen::Vector3d(), 1.0)),
                               Eigen::Vector3d>);
  static_assert(std::is_same_v<decltype(to_unit(Eigen::Matrix3d(), 1.0)),
                               Eigen::Matrix3d>);
  static_assert(std::is_same_v<decltype(to_unit(Eigen::Affine3d(), 1.0)),
                               Eigen::Affine3d>);
  static_assert(
      std::is_same_v<decltype(to_unit(Translation(), 1.0)), Translation>);
}

TEST(ElementToUnitTest, double) {
  EXPECT_EQ(to_unit(0.123456, 0.1), 0.123456 * 0.1);
}

TEST(ElementToUnitTest, float) {
  EXPECT_EQ(to_unit(0.123f, 0.1), 0.123f * 0.1f);
}

TEST(ElementToUnitTest, int64) {
  EXPECT_EQ(to_unit(int64_t(1), 0.1), 0);
  EXPECT_EQ(to_unit(int64_t(5), 0.1), 1); // 0.5 rounds up
  EXPECT_EQ(to_unit(int64_t(1), 1e6), 1000000);
  EXPECT_EQ(to_unit(int64_t(1), 1e10), 10000000000);
  EXPECT_EQ(to_unit(int64_t(13140985739), 1), 13140985739);
}

TEST(ElementToUnitTest, int32) {
  EXPECT_EQ(to_unit(int32_t(-100), 0.1), -10);
  EXPECT_EQ(to_unit(int32_t(-11), 0.1), -1);
  EXPECT_EQ(to_unit(int32_t(-10), 0.1), -1);
  EXPECT_EQ(to_unit(int32_t(-9), 0.1), -1);
  EXPECT_EQ(to_unit(int32_t(-5), 0.1), -1);
  EXPECT_EQ(to_unit(int32_t(-4), 0.1), 0);
  EXPECT_EQ(to_unit(int32_t(1), 0.1), 0);
  EXPECT_EQ(to_unit(int32_t(5), 0.1), 1); // 0.5 rounds up
  EXPECT_EQ(to_unit(int32_t(1), 1e6), 1000000);
  EXPECT_EQ(to_unit(std::numeric_limits<int32_t>::max(), 1),
            std::numeric_limits<int32_t>::max());
}

TEST(ElementToUnitTest, int_range_exceeded) {
  // We could consider using something like boost::numeric_cast to avoid this,
  // but it is not clear whether throwing exceptions on a per-element basis
  // would be desirable?
  EXPECT_EQ(to_unit(int32_t(1), 1e10), std::numeric_limits<int32_t>::max());
  EXPECT_EQ(to_unit(int32_t(-1), 1e10), std::numeric_limits<int32_t>::min());
  EXPECT_EQ(to_unit(int64_t(1), 1e20), std::numeric_limits<int64_t>::max());
  EXPECT_EQ(to_unit(int64_t(-1), 1e20), std::numeric_limits<int64_t>::min());
}

TEST(ElementToUnitTest, time_point) {
  EXPECT_EQ(to_unit(time_point(0), 0.1), time_point(0));
  EXPECT_EQ(to_unit(time_point(5), 0.1), time_point(1)); // 0.5 rounds up
  EXPECT_EQ(to_unit(time_point(1), 1e6), time_point(1000000));
}

TEST(ElementToUnitTest, vector3d) {
  const Eigen::Vector3d expected{10, 20, 30};
  EXPECT_EQ(to_unit(Eigen::Vector3d{1, 2, 3}, 10), expected);
}

TEST(ElementToUnitTest, affine3d) {
  const Eigen::AngleAxisd rotation(10.0, Eigen::Vector3d{1, 0, 0});
  const Eigen::Translation3d translation(2, 3, 4);
  const Eigen::Affine3d affine = rotation * translation;

  const Eigen::Translation3d expected_translation(20, 30, 40);
  const Eigen::Affine3d expected = rotation * expected_translation;

  EXPECT_TRUE(to_unit(affine, 10).isApprox(expected));
}

TEST(ElementToUnitTest, translation) {
  const Translation trans(Eigen::Vector3d{1, 2, 3});
  const Translation expected(Eigen::Vector3d{10, 20, 30});
  EXPECT_EQ(to_unit(trans, 10), expected);
}
