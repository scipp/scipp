// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "../element_geometric_operations.h"
#include "fix_typed_test_suite_warnings.h"
#include "scipp/units/unit.h"
#include <Eigen/Geometry>

using namespace scipp;
using namespace scipp::core::element;

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

template <typename T> class ElementPositionNTest : public ::testing::Test {};

template <int I> using component = geometry::detail::component<I>;

using types = ::testing::Types<component<0>, component<1>, component<2>>;
TYPED_TEST_SUITE(ElementPositionNTest, types);

TYPED_TEST(ElementPositionNTest, unzip_position) {
  using T = TypeParam;
  constexpr auto component = T::overloads;
  Eigen::Vector3d a{1.0, 2.0, 3.0};
  units::Unit m(units::m);
  units::Unit K(units::K);
  EXPECT_EQ(component(a), a[T::value]);
  EXPECT_EQ(geometry::detail::component<T::value>::overloads(m), m);
  EXPECT_THROW(geometry::z(K), except::UnitError);
}

TEST(ElementRotationTest, rotate_vector) {
  Eigen::Vector3d vec(1, 2, 3);
  Eigen::Quaterniond rot(4, 5, 6, 7);
  rot.normalize();
  // Rely on correctness of Eigen
  EXPECT_EQ(rot._transformVector(vec), geometry::rotate(vec, rot));
}
