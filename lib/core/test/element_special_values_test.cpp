// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/core/element/special_values.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"

using namespace scipp;
using namespace scipp::core;

namespace {
template <typename T, typename Op> void test_int_to_false_with_op(Op op) {
  EXPECT_FALSE(op(std::numeric_limits<T>::min()));
  EXPECT_FALSE(op(std::numeric_limits<T>::max()));
  EXPECT_FALSE(op(T{0}));
}
template <typename T, typename Op> void test_int_to_true_with_op(Op op) {
  EXPECT_TRUE(op(std::numeric_limits<T>::min()));
  EXPECT_TRUE(op(std::numeric_limits<T>::max()));
  EXPECT_TRUE(op(T{0}));
}
} // namespace

using ElementSpecialValuesTestTypes = ::testing::Types<double, float>;

template <typename T> class ElementIsnanTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementIsnanTest, ElementSpecialValuesTestTypes);

TEST(ElementIsnanTest, unit) {
  for (const auto &u : {sc_units::dimensionless, sc_units::m, sc_units::meV}) {
    EXPECT_EQ(element::isnan(u), sc_units::none);
  }
}

TYPED_TEST(ElementIsnanTest, value) {
  for (const auto x : {static_cast<TypeParam>(NAN),
                       std::numeric_limits<TypeParam>::quiet_NaN(),
                       std::numeric_limits<TypeParam>::signaling_NaN()}) {
    EXPECT_TRUE(element::isnan(x));
  }
  for (const auto x : {static_cast<TypeParam>(0.0), static_cast<TypeParam>(1.0),
                       std::numeric_limits<TypeParam>::infinity()}) {
    EXPECT_FALSE(element::isnan(x));
  }
}

TYPED_TEST(ElementIsnanTest, int_values) {
  test_int_to_false_with_op<int32_t>(element::isnan);
  test_int_to_false_with_op<int64_t>(element::isnan);
}

template <typename T> class ElementIsinfTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementIsinfTest, ElementSpecialValuesTestTypes);

TEST(ElementIsinfTest, unit) {
  for (const auto &u : {sc_units::dimensionless, sc_units::m, sc_units::meV}) {
    EXPECT_EQ(element::isinf(u), sc_units::none);
  }
}

TYPED_TEST(ElementIsinfTest, value) {
  for (const auto x : {std::numeric_limits<TypeParam>::infinity(),
                       -std::numeric_limits<TypeParam>::infinity()}) {
    EXPECT_TRUE(element::isinf(x));
  }
  for (const auto x : {static_cast<TypeParam>(0.0), static_cast<TypeParam>(1.0),
                       std::numeric_limits<TypeParam>::quiet_NaN(),
                       std::numeric_limits<TypeParam>::signaling_NaN()}) {
    EXPECT_FALSE(element::isinf(x));
  }
}

TYPED_TEST(ElementIsinfTest, int_values) {
  test_int_to_false_with_op<int32_t>(element::isinf);
  test_int_to_false_with_op<int64_t>(element::isinf);
}

template <typename T> class ElementIsfiniteTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementIsfiniteTest, ElementSpecialValuesTestTypes);

TEST(ElementIsfiniteTest, unit) {
  for (const auto &u : {sc_units::dimensionless, sc_units::m, sc_units::meV}) {
    EXPECT_EQ(element::isfinite(u), sc_units::none);
  }
}

TYPED_TEST(ElementIsfiniteTest, value) {
  for (const auto x : {static_cast<TypeParam>(0.0), static_cast<TypeParam>(3.4),
                       static_cast<TypeParam>(-1.0e3)}) {
    EXPECT_TRUE(element::isfinite(x));
  }
  for (const auto x : {std::numeric_limits<TypeParam>::infinity(),
                       std::numeric_limits<TypeParam>::quiet_NaN(),
                       std::numeric_limits<TypeParam>::signaling_NaN()}) {
    EXPECT_FALSE(element::isfinite(x));
  }
  EXPECT_TRUE(element::isfinite(1));
}

TYPED_TEST(ElementIsfiniteTest, int_values) {
  test_int_to_true_with_op<int32_t>(element::isfinite);
  test_int_to_true_with_op<int64_t>(element::isfinite);
}

template <typename T> class ElementIssignedinfTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementIssignedinfTest, ElementSpecialValuesTestTypes);

TEST(ElementIssignedinfTest, unit) {
  for (const auto &u : {sc_units::dimensionless, sc_units::m, sc_units::meV}) {
    EXPECT_EQ(element::isposinf(u), sc_units::none);
    EXPECT_EQ(element::isneginf(u), sc_units::none);
  }
}

TYPED_TEST(ElementIssignedinfTest, int_values) {
  test_int_to_false_with_op<int32_t>(element::isposinf);
  test_int_to_false_with_op<int64_t>(element::isposinf);
  test_int_to_false_with_op<int32_t>(element::isneginf);
  test_int_to_false_with_op<int64_t>(element::isneginf);
}

TYPED_TEST(ElementIssignedinfTest, value) {
  {
    const auto inf = std::numeric_limits<TypeParam>::infinity();
    EXPECT_TRUE(element::isposinf(inf));
    EXPECT_FALSE(element::isneginf(inf));
    EXPECT_TRUE(element::isneginf(-inf));
    EXPECT_FALSE(element::isposinf(-inf));
  }
  for (const auto x : {static_cast<TypeParam>(0.0), static_cast<TypeParam>(1.0),
                       std::numeric_limits<TypeParam>::quiet_NaN(),
                       std::numeric_limits<TypeParam>::signaling_NaN()}) {
    EXPECT_FALSE(element::isposinf(x));
    EXPECT_FALSE(element::isneginf(x));
  }
}

template <typename T> class ElementNanToNumTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementNanToNumTest, ElementSpecialValuesTestTypes);

template <typename T, typename Op>
void targetted_replacement_test(Op op, const T &replaceable,
                                const T &nonreplaceable, const T &replacement) {
  EXPECT_EQ(replacement, op(replaceable, replacement));
  EXPECT_EQ(nonreplaceable, op(nonreplaceable,
                               replacement)); // No replacement expected
}
template <typename T, typename Op>
void targetted_replacement_out_arg_test(Op op, T &out, const T &replaceable,
                                        const T &nonreplaceable,
                                        const T &replacement) {
  op(out, replaceable, replacement);
  EXPECT_EQ(replacement, out);
  op(out, nonreplaceable, replacement);
  EXPECT_EQ(nonreplaceable, out);
}

template <typename Op> void targetted_unit_test(Op op) {
  sc_units::Unit m(sc_units::m);
  EXPECT_EQ(m, op(m, m));
  sc_units::Unit s(sc_units::s);
  EXPECT_THROW(op(s, m), except::UnitError);
}

template <typename Op> void targetted_unit_test_out(Op op) {
  sc_units::Unit m(sc_units::m);
  sc_units::Unit u;
  op(u, m, m);
  EXPECT_EQ(m, u);
  sc_units::Unit s(sc_units::s);
  EXPECT_THROW(op(u, s, m), except::UnitError);
}

TYPED_TEST(ElementNanToNumTest, unit) {
  targetted_unit_test(element::nan_to_num);
}

TYPED_TEST(ElementNanToNumTest, value) {
  using T = TypeParam;
  const T replaceable = NAN;
  const T replacement = 1.0;
  const T nonreplaceable = 2.0;
  targetted_replacement_test(element::nan_to_num, replaceable, nonreplaceable,
                             replacement);
}

TYPED_TEST(ElementNanToNumTest, value_and_variance) {
  using T = TypeParam;
  const ValueAndVariance<T> replaceable(NAN, 0.1);
  const ValueAndVariance<T> replacement(1, 1);
  const ValueAndVariance<T> nonreplaceable(2, 2);
  targetted_replacement_test(element::nan_to_num, replaceable, nonreplaceable,
                             replacement);
}

TYPED_TEST(ElementNanToNumTest, unit_out) {
  targetted_unit_test_out(element::nan_to_num_out_arg);
}

TYPED_TEST(ElementNanToNumTest, value_out) {
  using T = TypeParam;
  const T replaceable = NAN;
  const T replacement = 1;
  const T nonreplaceable = 2;
  T out = -1;
  targetted_replacement_out_arg_test(element::nan_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}
TYPED_TEST(ElementNanToNumTest, value_and_variance_out) {
  using T = TypeParam;
  ValueAndVariance<T> replaceable(NAN, 2);
  const ValueAndVariance<T> nonreplaceable(3, 3);
  ValueAndVariance<T> out(-1, -1);
  const ValueAndVariance<T> replacement(1, 1);
  targetted_replacement_out_arg_test(element::nan_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}

template <typename T>
class ElementPositiveInfToNumTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementPositiveInfToNumTest, ElementSpecialValuesTestTypes);

TYPED_TEST(ElementPositiveInfToNumTest, unit) {
  targetted_unit_test(element::positive_inf_to_num);
}

TYPED_TEST(ElementPositiveInfToNumTest, value) {
  using T = TypeParam;
  const T replacement = 1.0;
  const T replaceable = INFINITY;
  const T nonreplaceable = -INFINITY;
  targetted_replacement_test(element::positive_inf_to_num, replaceable,
                             nonreplaceable, replacement);
}

TYPED_TEST(ElementPositiveInfToNumTest, value_and_variance) {
  using T = TypeParam;
  const ValueAndVariance<T> replaceable(INFINITY, 1);
  const ValueAndVariance<T> replacement(1, 1);
  const ValueAndVariance<T> nonreplaceable(-INFINITY, 1);
  targetted_replacement_test(element::positive_inf_to_num, replaceable,
                             nonreplaceable, replacement);
}
TYPED_TEST(ElementPositiveInfToNumTest, unit_out) {
  targetted_unit_test_out(element::positive_inf_to_num_out_arg);
}
TYPED_TEST(ElementPositiveInfToNumTest, value_out) {
  using T = TypeParam;
  T out = -1;
  const T replaceable = INFINITY;
  const T replacement = 1;
  const T nonreplaceable = -INFINITY;
  targetted_replacement_out_arg_test(element::positive_inf_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}
TYPED_TEST(ElementPositiveInfToNumTest, value_and_variance_out) {
  using T = TypeParam;
  ValueAndVariance<T> replaceable(INFINITY, 2);
  const ValueAndVariance<T> nonreplaceable(-INFINITY, 3);
  ValueAndVariance<T> out(-1, -1);
  const ValueAndVariance<T> replacement(1, 1);
  targetted_replacement_out_arg_test(element::positive_inf_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}

template <typename T>
class ElementNegativeInfToNumTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementNegativeInfToNumTest, ElementSpecialValuesTestTypes);

TYPED_TEST(ElementNegativeInfToNumTest, unit) {
  targetted_unit_test(element::negative_inf_to_num);
}
TYPED_TEST(ElementNegativeInfToNumTest, value) {
  using T = TypeParam;
  const T replacement = 1.0;
  const T replaceable = -INFINITY;
  const T nonreplaceable = INFINITY;
  targetted_replacement_test(element::negative_inf_to_num, replaceable,
                             nonreplaceable, replacement);
}

TYPED_TEST(ElementNegativeInfToNumTest, value_and_variance) {
  using T = TypeParam;
  const ValueAndVariance<T> replaceable(-INFINITY, 1);
  const ValueAndVariance<T> replacement(1, 1);
  const ValueAndVariance<T> nonreplaceable(INFINITY, 1);
  targetted_replacement_test(element::negative_inf_to_num, replaceable,
                             nonreplaceable, replacement);
}

TYPED_TEST(ElementNegativeInfToNumTest, unit_out) {
  targetted_unit_test_out(element::negative_inf_to_num_out_arg);
}
TYPED_TEST(ElementNegativeInfToNumTest, value_out) {
  using T = TypeParam;
  T out = -1;
  const T replaceable = -INFINITY;
  const T replacement = 1;
  const T nonreplaceable = INFINITY;
  targetted_replacement_out_arg_test(element::negative_inf_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}
TYPED_TEST(ElementNegativeInfToNumTest, value_and_variance_out) {
  using T = TypeParam;
  ValueAndVariance<T> replaceable(-INFINITY, 2);
  const ValueAndVariance<T> nonreplaceable(INFINITY, 3);
  ValueAndVariance<T> out(-1, -1);
  const ValueAndVariance<T> replacement(1, 1);
  targetted_replacement_out_arg_test(element::negative_inf_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}

TEST(ElementSpecialValueVector3dTest, isnan_true_if_one_element_is_nan) {
  Eigen::Vector3d v(1.0, 2.0, std::numeric_limits<double>::quiet_NaN());
  EXPECT_TRUE(element::isnan(v));
}

TEST(ElementSpecialValueVector3dTest, isnan_false_if_no_element_is_nan) {
  Eigen::Vector3d v(1.0, 2.0, 3.0);
  EXPECT_FALSE(element::isnan(v));
}

TEST(ElementSpecialValueVector3dTest, isfinite_false_if_one_element_is_nan) {
  Eigen::Vector3d v(1.0, 2.0, std::numeric_limits<double>::quiet_NaN());
  EXPECT_FALSE(element::isfinite(v));
}

TEST(ElementSpecialValueVector3dTest, isfinite_true_if_no_element_is_nan) {
  Eigen::Vector3d v(1.0, 2.0, 3.0);
  EXPECT_TRUE(element::isfinite(v));
}

TEST(ElementSpecialValueVector3dTest, isinf_false_if_one_element_is_nan) {
  Eigen::Vector3d v(1.0, 2.0, std::numeric_limits<double>::quiet_NaN());
  EXPECT_FALSE(element::isinf(v));
}

TEST(ElementSpecialValueVector3dTest, isnan_false_if_one_element_is_inf) {
  Eigen::Vector3d v(1.0, 2.0, std::numeric_limits<double>::infinity());
  EXPECT_FALSE(element::isnan(v));
}

TEST(ElementSpecialValueVector3dTest, isfinite_false_if_one_element_is_inf) {
  Eigen::Vector3d v(1.0, 2.0, std::numeric_limits<double>::infinity());
  EXPECT_FALSE(element::isfinite(v));
}

TEST(ElementSpecialValueVector3dTest, isfinite_true_if_no_element_is_inf) {
  Eigen::Vector3d v(1.0, 2.0, 3.0);
  EXPECT_TRUE(element::isfinite(v));
}

TEST(ElementSpecialValueVector3dTest, isinf_true_if_one_element_is_inf) {
  Eigen::Vector3d v(1.0, 2.0, std::numeric_limits<double>::infinity());
  EXPECT_TRUE(element::isinf(v));
}

TEST(ElementSpecialValueVector3dTest, isinf_false_if_no_element_is_inf) {
  Eigen::Vector3d v(1.0, 2.0, 3.0);
  EXPECT_FALSE(element::isinf(v));
}
