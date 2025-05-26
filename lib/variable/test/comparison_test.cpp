// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "scipp/core/eigen.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/structures.h"
#include "test_macros.h"
#include <gtest/gtest.h>

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::sc_units;

template <typename T> class IsCloseTest : public ::testing::Test {};
using TestTypes = ::testing::Types<double, float, int64_t, int32_t>;

TYPED_TEST_SUITE(IsCloseTest, TestTypes);

TYPED_TEST(IsCloseTest, atol_when_variable_equal) {
  const auto a = makeVariable<TypeParam>(Values{1});
  const auto rtol = makeVariable<TypeParam>(Values{0});
  const auto atol = makeVariable<TypeParam>(Values{1});
  EXPECT_EQ(isclose(a, a, rtol, atol), true * sc_units::none);
}

TYPED_TEST(IsCloseTest, atol_when_variables_within_tolerance) {
  const auto a = makeVariable<TypeParam>(Values{0});
  const auto b = makeVariable<TypeParam>(Values{1});
  const auto rtol = makeVariable<TypeParam>(Values{0});
  const auto atol = makeVariable<TypeParam>(Values{1});
  EXPECT_EQ(isclose(a, b, rtol, atol), true * sc_units::none);
}

TYPED_TEST(IsCloseTest, atol_when_variables_outside_tolerance) {
  const auto a = makeVariable<TypeParam>(Values{0});
  const auto b = makeVariable<TypeParam>(Values{2});
  const auto rtol = makeVariable<TypeParam>(Values{0});
  const auto atol = makeVariable<TypeParam>(Values{1});
  EXPECT_EQ(isclose(a, b, rtol, atol), false * sc_units::none);
}

TYPED_TEST(IsCloseTest, rtol_when_variables_within_tolerance) {
  const auto a = makeVariable<TypeParam>(Values{8});
  const auto b = makeVariable<TypeParam>(Values{9});
  // tol = atol + rtol * b = 1
  const auto rtol = makeVariable<double>(Values{1.0 / 9});
  const auto atol = makeVariable<TypeParam>(Values{0});
  EXPECT_EQ(isclose(a, b, rtol, atol), true * sc_units::none);
}

TYPED_TEST(IsCloseTest, rtol_when_variables_outside_tolerance) {
  const auto a = makeVariable<TypeParam>(Values{7});
  const auto b = makeVariable<TypeParam>(Values{9});
  // tol = atol + rtol * b = 1
  const auto rtol = makeVariable<double>(Values{1.0 / 9});
  const auto atol = makeVariable<TypeParam>(Values{0});
  EXPECT_EQ(isclose(a, b, rtol, atol), false * sc_units::none);
}

TEST(IsCloseTest, with_vectors) {
  const auto u =
      makeVariable<Eigen::Vector3d>(Values{Eigen::Vector3d{0, 0, 0}});
  const auto v =
      makeVariable<Eigen::Vector3d>(Values{Eigen::Vector3d{1, 1, 1}});
  const auto w =
      makeVariable<Eigen::Vector3d>(Values{Eigen::Vector3d{1, 1, 1.0001}});
  const auto rtol = 0.0 * sc_units::one;
  const auto atol = 1.0 * sc_units::one;
  EXPECT_EQ(isclose(u, u, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, v, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(v, w, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, w, rtol, atol), makeVariable<bool>(Values{false}));
}

TEST(IsCloseTest, with_matrices) {
  const Eigen::Matrix3d mat_u = Eigen::Matrix3d::Constant(3, 3, 0.0);
  const auto u = makeVariable<Eigen::Matrix3d>(Values{mat_u});
  const Eigen::Matrix3d mat_v = Eigen::Matrix3d::Constant(3, 3, 1.0);
  const auto v = makeVariable<Eigen::Matrix3d>(Values{mat_v});
  const Eigen::Matrix3d mat_w = Eigen::Matrix3d::Constant(3, 3, 1.0001);
  const auto w = makeVariable<Eigen::Matrix3d>(Values{mat_w});
  const auto rtol = 0.0 * sc_units::one;
  const auto atol = 1.0 * sc_units::one;
  EXPECT_EQ(isclose(u, u, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, v, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(v, w, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, w, rtol, atol), makeVariable<bool>(Values{false}));
}

TEST(IsCloseTest, with_affine) {
  // The interaction of rotation and translation is non-trivial.
  // we set angle=0 to help pick a meaningful atol.
  const Eigen::AngleAxisd u_rotation(0, Eigen::Vector3d{0, 1, 0});
  const Eigen::Translation3d u_translation(-4, 1, 3);
  const Eigen::Affine3d u_affine = u_rotation * u_translation;
  const auto u = makeVariable<Eigen::Affine3d>(Values{u_affine});

  const Eigen::AngleAxisd v_rotation(0, Eigen::Vector3d{0, 1, 0});
  const Eigen::Translation3d v_translation(-5, 2, 2);
  const Eigen::Affine3d v_affine = v_rotation * v_translation;
  const auto v = makeVariable<Eigen::Affine3d>(Values{v_affine});

  const Eigen::AngleAxisd w_rotation(0, Eigen::Vector3d{0, 1, 0});
  const Eigen::Translation3d w_translation(-5, 2, 1.9999);
  const Eigen::Affine3d w_affine = w_rotation * w_translation;
  const auto w = makeVariable<Eigen::Affine3d>(Values{w_affine});

  const auto rtol = 0.0 * sc_units::one;
  const auto atol = 1.0 * sc_units::one;
  EXPECT_EQ(isclose(u, u, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, v, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(v, w, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, w, rtol, atol), makeVariable<bool>(Values{false}));
}

TEST(IsCloseTest, with_translation) {
  const auto u = makeVariable<core::Translation>(
      Values{core::Translation{Eigen::Vector3d{0, 0, 0}}});
  const auto v = makeVariable<core::Translation>(
      Values{core::Translation{Eigen::Vector3d{1, 1, 1}}});
  const auto w = makeVariable<core::Translation>(
      Values{core::Translation{Eigen::Vector3d{1, 1, 1.0001}}});
  const auto rtol = 0.0 * sc_units::one;
  const auto atol = 1.0 * sc_units::one;
  EXPECT_EQ(isclose(u, u, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, v, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(v, w, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, w, rtol, atol), makeVariable<bool>(Values{false}));
}

TEST(IsCloseTest, with_quaternion) {
  const auto u = makeVariable<core::Quaternion>(
      Values{core::Quaternion{Eigen::Quaterniond{0, 0, 0, 0}}});
  const auto v = makeVariable<core::Quaternion>(
      Values{core::Quaternion{Eigen::Quaterniond{1, -1, 0.5, -0.25}}});
  const auto w = makeVariable<core::Quaternion>(
      Values{core::Quaternion{Eigen::Quaterniond{1, -1, 0.5, -1.2}}});
  const auto rtol = 0.0 * sc_units::one;
  const auto atol = 1.0 * sc_units::one;
  EXPECT_EQ(isclose(u, u, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, v, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(v, w, rtol, atol), makeVariable<bool>(Values{true}));
  EXPECT_EQ(isclose(u, w, rtol, atol), makeVariable<bool>(Values{false}));
}

TEST(IsCloseTest, works_for_counts) {
  const auto a =
      makeVariable<double>(Values{1}, Variances{1}, sc_units::counts);
  const auto rtol = 1e-5 * sc_units::one;
  const auto atol = 0.0 * sc_units::counts;
  EXPECT_NO_THROW_DISCARD(isclose(a, a, rtol, atol));
}

TEST(IsCloseTest, compare_variances_only) {
  // Tests setup so that value comparison does not affect output (a, b value
  // same)
  const auto a = makeVariable<double>(Values{10.0}, Variances{0.0});
  const auto b = makeVariable<double>(Values{10.0}, Variances{1.0});
  EXPECT_EQ(isclose(a, b, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{1.0})),
            true * sc_units::none);
  EXPECT_EQ(isclose(a, b, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{0.9})),
            false * sc_units::none);
}

TEST(IsCloseTest, compare_values_and_variances) {
  // Tests setup so that value comparison does not affect output (a, b value
  // same)
  const auto w = makeVariable<double>(Values{10.0}, Variances{0.0});
  const auto x = makeVariable<double>(Values{9.0}, Variances{0.0});
  const auto y = makeVariable<double>(Values{10.0}, Variances{1.0});
  const auto z = makeVariable<double>(Values{9.0}, Variances{1.0});
  // sanity check no mismatch
  EXPECT_EQ(isclose(w, w, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{0.9})),
            true * sc_units::none);
  // mismatch value only
  EXPECT_EQ(isclose(w, x, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{0.9})),
            false * sc_units::none);
  // mismatch variance only
  EXPECT_EQ(isclose(w, y, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{0.9})),
            false * sc_units::none);
  // mismatch value and variance
  EXPECT_EQ(isclose(w, z, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{0.9})),
            false * sc_units::none);

  // same as above but looser tolerance
  EXPECT_EQ(isclose(w, z, makeVariable<double>(Values{0.0}),
                    makeVariable<double>(Values{1.0})),
            true * sc_units::none);
}

TEST(IsCloseTest, rtol_units) {
  auto unit = scipp::sc_units::m;
  const auto a = makeVariable<double>(Values{1.0}, Variances{1.0}, unit);
  // This is fine
  EXPECT_EQ(isclose(a, a, 1.0 * scipp::sc_units::one, 1.0 * unit),
            true * scipp::sc_units::none);
  // Now rtol has units m
  EXPECT_THROW_DISCARD(isclose(a, a, 1.0 * unit, 1.0 * unit),
                       except::UnitError);
}

TEST(IsCloseTest, no_unit) {
  const auto a =
      makeVariable<double>(Values{1.0}, Variances{1.0}, sc_units::none);
  EXPECT_EQ(
      isclose(a, a, 1.0 * scipp::sc_units::none, 1.0 * scipp::sc_units::none),
      true * scipp::sc_units::none);
  EXPECT_THROW_DISCARD(isclose(a, a, 1.0 * scipp::sc_units::dimensionless,
                               1.0 * scipp::sc_units::none),
                       except::UnitError);
  const auto b = makeVariable<double>(Values{1.0}, Variances{1.0},
                                      sc_units::dimensionless);
  EXPECT_THROW_DISCARD(isclose(b, b, 1.0 * scipp::sc_units::dimensionless,
                               1.0 * scipp::sc_units::none),
                       except::UnitError);
}

TEST(ComparisonTest, variances_test) {
  const auto a = makeVariable<float>(Values{1.0}, Variances{1.0});
  const auto b = makeVariable<float>(Values{2.0}, Variances{2.0});
  EXPECT_EQ(less(a, b), true * sc_units::none);
  EXPECT_EQ(less_equal(a, b), true * sc_units::none);
  EXPECT_EQ(greater(a, b), false * sc_units::none);
  EXPECT_EQ(greater_equal(a, b), false * sc_units::none);
  EXPECT_EQ(equal(a, b), false * sc_units::none);
  EXPECT_EQ(not_equal(a, b), true * sc_units::none);
}

TEST(ComparisonTest, can_broadcast_variances) {
  const auto a =
      makeVariable<float>(Dims{Dim::X}, Shape{1}, Values{1.0}, Variances{1.0});
  const auto b = makeVariable<float>(Values{2.0}, Variances{2.0});
  const auto True = makeVariable<bool>(Dims{Dim::X}, Shape{1}, Values{true});
  const auto False = ~True;
  EXPECT_EQ(less(a, b), True);
  EXPECT_EQ(less_equal(a, b), True);
  EXPECT_EQ(greater(a, b), False);
  EXPECT_EQ(greater_equal(a, b), False);
  EXPECT_EQ(equal(a, b), False);
  EXPECT_EQ(not_equal(a, b), True);
}

TEST(ComparisonTest, less_units_test) {
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0.0, 3.0});
  b.setUnit(sc_units::m);
  EXPECT_THROW([[maybe_unused]] auto out = less(a, b), std::runtime_error);
}

namespace {
const auto a = 1.0 * sc_units::m;
const auto b = 2.0 * sc_units::m;
const auto sa = makeVariable<std::string>(Dims{}, Values{"a"});
const auto sb = makeVariable<std::string>(Dims{}, Values{"b"});
const auto va = make_vectors(Dimensions{}, sc_units::m, {1.0, 2.0, 3.0});
const auto vb = make_vectors(Dimensions{}, sc_units::m, {4.0, 5.0, 6.0});
const auto ma = make_matrices(Dimensions{}, sc_units::m,
                              {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});
const auto mb = make_matrices(Dimensions{}, sc_units::m,
                              {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9});
const auto true_ = true * sc_units::none;
const auto false_ = false * sc_units::none;
} // namespace

TEST(ComparisonTest, less_test) {
  EXPECT_EQ(less(a, b), true_);
  EXPECT_EQ(less(b, a), false_);
  EXPECT_EQ(less(a, a), false_);
}
TEST(ComparisonTest, greater_test) {
  EXPECT_EQ(greater(a, b), false_);
  EXPECT_EQ(greater(b, a), true_);
  EXPECT_EQ(greater(a, a), false_);
}
TEST(ComparisonTest, greater_equal_test) {
  EXPECT_EQ(greater_equal(a, b), false_);
  EXPECT_EQ(greater_equal(b, a), true_);
  EXPECT_EQ(greater_equal(a, a), true_);
}
TEST(ComparisonTest, less_equal_test) {
  EXPECT_EQ(less_equal(a, b), true_);
  EXPECT_EQ(less_equal(b, a), false_);
  EXPECT_EQ(less_equal(a, a), true_);
}
TEST(ComparisonTest, equal_test) {
  EXPECT_EQ(equal(a, b), false_);
  EXPECT_EQ(equal(b, a), false_);
  EXPECT_EQ(equal(a, a), true_);
}
TEST(ComparisonTest, equal_test_string) {
  EXPECT_EQ(equal(sa, sb), false_);
  EXPECT_EQ(equal(sb, sa), false_);
  EXPECT_EQ(equal(sa, sa), true_);
}
TEST(ComparisonTest, equal_test_vector) {
  EXPECT_EQ(equal(va, vb), false_);
  EXPECT_EQ(equal(vb, va), false_);
  EXPECT_EQ(equal(va, va), true_);
}
TEST(ComparisonTest, equal_test_matrix) {
  EXPECT_EQ(equal(ma, mb), false_);
  EXPECT_EQ(equal(mb, ma), false_);
  EXPECT_EQ(equal(ma, ma), true_);
}
TEST(ComparisonTest, not_equal_test) {
  EXPECT_EQ(not_equal(a, b), true_);
  EXPECT_EQ(not_equal(b, a), true_);
  EXPECT_EQ(not_equal(a, a), false_);
}
TEST(ComparisonTest, not_equal_test_string) {
  EXPECT_EQ(not_equal(sa, sb), true_);
  EXPECT_EQ(not_equal(sb, sa), true_);
  EXPECT_EQ(not_equal(sa, sa), false_);
}
TEST(ComparisonTest, not_equal_test_vector) {
  EXPECT_EQ(not_equal(va, vb), true_);
  EXPECT_EQ(not_equal(vb, va), true_);
  EXPECT_EQ(not_equal(va, va), false_);
}
TEST(ComparisonTest, not_equal_test_matrix) {
  EXPECT_EQ(not_equal(ma, mb), true_);
  EXPECT_EQ(not_equal(mb, ma), true_);
  EXPECT_EQ(not_equal(ma, ma), false_);
}
