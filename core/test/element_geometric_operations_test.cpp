// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <Eigen/Geometry>

#include "scipp/common/constants.h"
#include "scipp/core/element/geometric_operations.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"

using namespace scipp;
using namespace scipp::core::element;

TEST(ElementPositionTest, unit_in) {
  const units::Unit m(units::m);
  const units::Unit s(units::s); // Not supported
  EXPECT_THROW(geometry::position(s, m, m), except::UnitError);
  EXPECT_THROW(geometry::position(m, s, m), except::UnitError);
  EXPECT_THROW(geometry::position(m, m, s), except::UnitError);
  EXPECT_NO_THROW(geometry::position(s, s, s));
  EXPECT_NO_THROW(geometry::position(m, m, m));
}

TEST(ElementPositionTest, unit_out) {
  const units::Unit m(units::m);
  EXPECT_EQ(geometry::position(m, m, m), m);
}

TEST(ElementPositionTest, zip_position_values) {
  EXPECT_EQ((Eigen::Vector3d(1, 2, 3)), geometry::position(1.0, 2.0, 3.0));
}

TEST(ElementPositionNTest, unzip_position) {
  Eigen::Vector3d a{1.0, 2.0, 3.0};
  EXPECT_EQ(geometry::x(a), a[0]);
  EXPECT_EQ(geometry::y(a), a[1]);
  EXPECT_EQ(geometry::z(a), a[2]);
  EXPECT_EQ(geometry::x(units::m), units::m);
  EXPECT_EQ(geometry::y(units::m), units::m);
  EXPECT_EQ(geometry::z(units::m), units::m);
}

TEST(ElementRotationTest, rotate_vector) {
  // With human readable rotation
  Eigen::Quaterniond rot1(
      Eigen::AngleAxisd(-0.5 * scipp::pi<double>, Eigen::Vector3d::UnitY()));
  EXPECT_TRUE(geometry::rotate(Eigen::Vector3d::UnitX(), rot1)
                  .isApprox(Eigen::Vector3d::UnitZ(),
                            2.0 * std::numeric_limits<double>::epsilon()));
  // With arbitrary rotation: rely on correctness of Eigen
  Eigen::Vector3d vec(1, 2, 3);
  Eigen::Quaterniond rot2(4, 5, 6, 7);
  rot2.normalize();
  EXPECT_EQ(rot2._transformVector(vec), geometry::rotate(vec, rot2));
}

TEST(ElementRotationTest, rotate_vector_inplace) {
  // With human readable rotation
  Eigen::Vector3d out(0, 0, 0);
  Eigen::Quaterniond rot1(
      Eigen::AngleAxisd(-0.5 * scipp::pi<double>, Eigen::Vector3d::UnitY()));
  geometry::rotate_out_arg(out, Eigen::Vector3d::UnitX(), rot1);
  EXPECT_TRUE(out.isApprox(Eigen::Vector3d::UnitZ(),
                           2.0 * std::numeric_limits<double>::epsilon()));
  // With arbitrary rotation: rely on correctness of Eigen
  Eigen::Vector3d vec(1, 2, 3);
  Eigen::Quaterniond rot2(4, 5, 6, 7);
  rot2.normalize();
  geometry::rotate_out_arg(out, vec, rot2);
  EXPECT_EQ(rot2._transformVector(vec), out);
}

TEST(ElementRotationTest, unit_out) {
  const units::Unit m(units::m);
  const units::Unit dimless(units::dimensionless);
  EXPECT_EQ(geometry::rotate(m, dimless), m);
  units::Unit u_out(units::dimensionless);
  const units::Unit a(units::angstrom);
  geometry::rotate_out_arg(u_out, a, dimless);
  EXPECT_EQ(a, u_out);
}
