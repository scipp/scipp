// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "../element_geometric_operations.h"
#include "fix_typed_test_suite_warnings.h"
#include "scipp/units/unit.h"
#include <Eigen/Geometry>

using namespace scipp;
using namespace scipp::core;
using namespace element;

TEST(ElementPositionTest, unit_in_m) {
  const units::Unit m(units::m);
  const units::Unit s(units::s); // Not supported
  EXPECT_THROW(geometry::position(s, m, m), except::UnitError);
  EXPECT_THROW(geometry::position(m, s, m), except::UnitError);
  EXPECT_THROW(geometry::position(m, m, s), except::UnitError);
  EXPECT_THROW(geometry::position(s, s, s), except::UnitError);
  EXPECT_NO_THROW(geometry::position(m, m, m));
}

TEST(ElementPositionTest, unit_out) {
  const units::Unit m(units::m);
  EXPECT_EQ(geometry::position(m, m, m), m);
}

TEST(ElementPositionTest, zip_position_values) {
  EXPECT_EQ((Eigen::Vector3d(1, 2, 3)), geometry::position(1.0, 2.0, 3.0));
}

TEST(ElementPositionTest, unzip_position_x) {
  Eigen::Vector3d a{1.0, 2.0, 3.0};
  units::Unit m(units::m);
  units::Unit K(units::K);
  EXPECT_EQ(geometry::x(a), 1.0);
  EXPECT_EQ(geometry::x(m), m);
  EXPECT_THROW(geometry::x(K), except::UnitError);
}

TEST(ElementPositionTest, unzip_position_y) {
  Eigen::Vector3d a{1.0, 2.0, 3.0};
  units::Unit m(units::m);
  units::Unit K(units::K);
  EXPECT_EQ(geometry::y(a), 2.0);
  EXPECT_EQ(geometry::y(m), m);
  EXPECT_THROW(geometry::y(K), except::UnitError);
}

TEST(ElementPositionTest, unzip_position_z) {
  Eigen::Vector3d a{1.0, 2.0, 3.0};
  units::Unit m(units::m);
  units::Unit K(units::K);
  EXPECT_EQ(geometry::z(a), 3.0);
  EXPECT_EQ(geometry::z(m), m);
  EXPECT_THROW(geometry::z(K), except::UnitError);
}
