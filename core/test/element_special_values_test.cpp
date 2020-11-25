// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/special_values.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"

using namespace scipp;
using namespace scipp::core;

using ElementSpecialValuesTestTypes = ::testing::Types<double, float>;

template <typename T> class ElementIsnanTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementIsnanTest, ElementSpecialValuesTestTypes);

TEST(ElementIsnanTest, unit) {
  for (const auto &u : {units::dimensionless, units::m, units::meV}) {
    EXPECT_EQ(element::isnan(u), units::dimensionless);
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

template <typename T> class ElementIsinfTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementIsinfTest, ElementSpecialValuesTestTypes);

TEST(ElementIsinfTest, unit) {
  for (const auto &u : {units::dimensionless, units::m, units::meV}) {
    EXPECT_EQ(element::isinf(u), units::dimensionless);
  }
}

TYPED_TEST(ElementIsinfTest, value) {
  for (const auto x :
       {static_cast<TypeParam>(1.0) / static_cast<TypeParam>(0.0),
        std::numeric_limits<TypeParam>::infinity(),
        -std::numeric_limits<TypeParam>::infinity()}) {
    EXPECT_TRUE(element::isinf(x));
  }
  for (const auto x : {static_cast<TypeParam>(0.0), static_cast<TypeParam>(1.0),
                       std::numeric_limits<TypeParam>::quiet_NaN(),
                       std::numeric_limits<TypeParam>::signaling_NaN()}) {
    EXPECT_FALSE(element::isinf(x));
  }
}

template <typename T> class ElementIsfiniteTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementIsfiniteTest, ElementSpecialValuesTestTypes);

TEST(ElementIsfiniteTest, unit) {
  for (const auto &u : {units::dimensionless, units::m, units::meV}) {
    EXPECT_EQ(element::isfinite(u), units::dimensionless);
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
}

template <typename T> class ElementIssignedinfTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementIssignedinfTest, ElementSpecialValuesTestTypes);

TEST(ElementIssignedinfTest, unit) {
  for (const auto &u : {units::dimensionless, units::m, units::meV}) {
    EXPECT_EQ(element::isposinf(u), units::dimensionless);
    EXPECT_EQ(element::isneginf(u), units::dimensionless);
  }
}

TYPED_TEST(ElementIssignedinfTest, value) {
  for (const auto x :
       {static_cast<TypeParam>(1.0) / static_cast<TypeParam>(0.0),
        std::numeric_limits<TypeParam>::infinity()}) {
    EXPECT_TRUE(element::isposinf(x));
    EXPECT_FALSE(element::isneginf(x));
    EXPECT_TRUE(element::isneginf(-x));
    EXPECT_FALSE(element::isposinf(-x));
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
  units::Unit m(units::m);
  EXPECT_EQ(m, op(m, m));
  units::Unit s(units::s);
  EXPECT_THROW(op(s, m), except::UnitError);
}

template <typename Op> void targetted_unit_test_out(Op op) {
  units::Unit m(units::m);
  units::Unit u;
  op(u, m, m);
  EXPECT_EQ(m, u);
  units::Unit s(units::s);
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
