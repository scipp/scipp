// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <Eigen/Geometry>

#include "scipp/common/constants.h"
<<<<<<< HEAD
#include "scipp/core/element_geometric_operations.h"
#include "scipp/core/value_and_variance.h"

=======
#include "scipp/core/element/geometric_operations.h"
>>>>>>> master
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

template <typename T> class ElementPositionNTest : public ::testing::Test {};

template <int I> using component = geometry::detail::component<I>;

using types = ::testing::Types<component<0>, component<1>, component<2>>;
TYPED_TEST_SUITE(ElementPositionNTest, types);

TYPED_TEST(ElementPositionNTest, unzip_position) {
  using T = TypeParam;
  constexpr auto component = T::overloads;
  Eigen::Vector3d a{1.0, 2.0, 3.0};
  units::Unit m(units::m);
  EXPECT_EQ(component(a), a[T::value]);
  EXPECT_EQ(geometry::detail::component<T::value>::overloads(m), m);
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

template <typename T> class ElementLessTest : public ::testing::Test {};
template <typename T> class ElementGreaterTest : public ::testing::Test {};
template <typename T> class ElementLessEqualTest : public ::testing::Test {};
template <typename T> class ElementGreaterEqualTest : public ::testing::Test {};
template <typename T> class ElementEqualTest : public ::testing::Test {};
template <typename T> class ElementNotEqualTest : public ::testing::Test {};
using ElementLessTestTypes = ::testing::Types<double, float, int64_t, int32_t>;
TYPED_TEST_SUITE(ElementLessTest, ElementLessTestTypes);
TYPED_TEST_SUITE(ElementGreaterTest, ElementLessTestTypes);
TYPED_TEST_SUITE(ElementLessEqualTest, ElementLessTestTypes);
TYPED_TEST_SUITE(ElementGreaterEqualTest, ElementLessTestTypes);
TYPED_TEST_SUITE(ElementEqualTest, ElementLessTestTypes);
TYPED_TEST_SUITE(ElementNotEqualTest, ElementLessTestTypes);

TYPED_TEST(ElementLessTest, unit) {
  const units::Unit m(units::m);
  EXPECT_EQ(less(m, m), units::dimensionless);
  const units::Unit rad(units::rad);
  EXPECT_THROW(less(rad, m), except::UnitError);
}

TYPED_TEST(ElementLessTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(less(y, x), true);
  x = -1;
  EXPECT_EQ(less(y, x), false);
}

template <int T, typename Op> bool is_no_variance_arg() {
  return std::is_base_of_v<core::transform_flags::expect_no_variance_arg_t<T>, Op>;
}

TYPED_TEST(ElementLessTest, value_only_arguments) {
  using Op = decltype(less);
  EXPECT_TRUE((is_no_variance_arg<0, Op>())) << " y has variance ";
  EXPECT_TRUE((is_no_variance_arg<1, Op>())) << " x has variance ";
}

TYPED_TEST(ElementGreaterTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(greater(y, x), false);
  x = -1;
  EXPECT_EQ(greater(y, x), true);
}

TYPED_TEST(ElementLessEqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(less_equal(y, x), true);
  x = 1;
  EXPECT_EQ(less_equal(y, x), true);
}

TYPED_TEST(ElementGreaterEqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(greater_equal(y, x), false);
  x = 1;
  EXPECT_EQ(greater_equal(y, x), true);
}

TYPED_TEST(ElementEqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(equal(y, x), false);
  x = 1;
  EXPECT_EQ(equal(y, x), true);
}

TYPED_TEST(ElementNotEqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(not_equal(y, x), true);
  x = 1;
  EXPECT_EQ(not_equal(y, x), false);
}
