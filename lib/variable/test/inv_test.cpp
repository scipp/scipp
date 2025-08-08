// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/spatial_transforms.h"
#include "scipp/variable/inv.h"

using namespace scipp;
using namespace scipp::variable;

const double TOLERANCE = pow(10, -10);

namespace {
bool is_close(const Variable &var, const Eigen::Vector3d &vec) {
  return var.value<Eigen::Vector3d>().isApprox(vec, TOLERANCE);
}
} // namespace

TEST(ElementInverseTest, linear_transform) {
  const Eigen::Matrix3d t({{0.1, 2.3, 1.7}, {3.1, 0.4, 0.6}, {0.9, 1.2, 1.6}});
  const auto transform =
      makeVariable<Eigen::Matrix3d>(Dims{}, Values{t}, sc_units::m);

  const Eigen::Vector3d v(0.1, 2.1, 1.4);
  const auto vec =
      makeVariable<Eigen::Vector3d>(Dims{}, Values{v}, sc_units::s);

  const auto rtol = makeVariable<double>(Values{0});
  const auto atol = makeVariable<Eigen::Vector3d>(
      Values{Eigen::Vector3d(TOLERANCE, TOLERANCE, TOLERANCE)});

  const auto res = inv(transform) * transform * vec;
  EXPECT_TRUE(is_close(res, v));
  EXPECT_EQ(res.unit(), vec.unit());
  EXPECT_EQ(res.dims(), vec.dims());
}

TEST(ElementInverseTest, affine_transform) {
  const Eigen::Matrix3d rotation{
      {0.1, 2.3, 1.7}, {3.1, 0.4, 0.6}, {0.9, 1.2, 1.6}};
  Eigen::Affine3d t(Eigen::Translation<double, 3>(Eigen::Vector3d(1, 2, 3)));
  t *= rotation;
  const auto transform =
      makeVariable<Eigen::Affine3d>(Dims{}, Values{t}, sc_units::m);

  const Eigen::Vector3d v(1.1, -5.2, 4.0);
  const auto vec =
      makeVariable<Eigen::Vector3d>(Dims{}, Values{v}, sc_units::m);

  const auto res = inv(transform) * transform * vec;
  EXPECT_TRUE(is_close(res, v));
  EXPECT_EQ(res.unit(), vec.unit());
  EXPECT_EQ(res.dims(), vec.dims());
}

TEST(ElementInverseTest, translation) {
  const core::Translation t(Eigen::Vector3d(4, 2, -3));
  const auto transform =
      makeVariable<core::Translation>(Dims{}, Values{t}, sc_units::s);

  const Eigen::Vector3d v(-0.2, 0.5, 11.2);
  const auto vec =
      makeVariable<Eigen::Vector3d>(Dims{}, Values{v}, sc_units::s);

  const auto res = inv(transform) * transform * vec;
  EXPECT_TRUE(is_close(res, v));
  EXPECT_EQ(res.unit(), vec.unit());
  EXPECT_EQ(res.dims(), vec.dims());
}

TEST(ElementInverseTest, rotation) {
  const core::Quaternion t(Eigen::Quaterniond(0.3, -0.5, 0.2, 1.2));
  const auto transform = makeVariable<core::Quaternion>(Dims{}, Values{t});

  const Eigen::Vector3d v(4.1, -4.1, -2.2);
  const auto vec =
      makeVariable<Eigen::Vector3d>(Dims{}, Values{v}, sc_units::kg);

  const auto res = inv(transform) * transform * vec;
  EXPECT_TRUE(is_close(res, v));
  EXPECT_EQ(res.unit(), vec.unit());
  EXPECT_EQ(res.dims(), vec.dims());
}
