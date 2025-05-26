// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <Eigen/Geometry>

#include "scipp/core/element/geometric_operations.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"

using namespace scipp;
using namespace scipp::core::element;

TEST(ElementPositionTest, unit_in) {
  const sc_units::Unit m(sc_units::m);
  const sc_units::Unit s(sc_units::s); // Not supported
  EXPECT_THROW(geometry::position(s, m, m), except::UnitError);
  EXPECT_THROW(geometry::position(m, s, m), except::UnitError);
  EXPECT_THROW(geometry::position(m, m, s), except::UnitError);
  EXPECT_NO_THROW(geometry::position(s, s, s));
  EXPECT_NO_THROW(geometry::position(m, m, m));
}

TEST(ElementPositionTest, unit_out) {
  const sc_units::Unit m(sc_units::m);
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
  EXPECT_EQ(geometry::x(sc_units::m), sc_units::m);
  EXPECT_EQ(geometry::y(sc_units::m), sc_units::m);
  EXPECT_EQ(geometry::z(sc_units::m), sc_units::m);
}
