// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "../element_geometric_operations.h"
#include "fix_typed_test_suite_warnings.h"
#include "scipp/units/unit.h"
#include <Eigen/Geometry>

using namespace scipp;
using namespace scipp::core;

TEST(ElementPositionTest, unit_in_m) {
  const units::Unit m(units::m);
  const units::Unit s(units::s); // Not supported
  EXPECT_THROW(element::position(s, m, m), except::UnitError);
  EXPECT_THROW(element::position(m, s, m), except::UnitError);
  EXPECT_THROW(element::position(m, m, s), except::UnitError);
  EXPECT_THROW(element::position(s, s, s), except::UnitError);
  EXPECT_NO_THROW(element::position(m, m, m));
}

TEST(ElementPositionTest, unit_out) {
  const units::Unit m(units::m);
  EXPECT_EQ(element::position(m, m, m), m);
}

TEST(ElementPositionTest, zip_position_values) {
  EXPECT_EQ((Eigen::Vector3d(1, 2, 3)), element::position(1.0, 2.0, 3.0));
  EXPECT_EQ((Eigen::Vector3d(1, 2, 3)), element::position(1.0f, 2.0f, 3.0f));
}
