// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/hyperbolic.h"

#include "fix_typed_test_suite_warnings.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::core;

/*
 * We only use approximate equality in some places to work around compiler
 * optimizations. In principle, element::sinh(2.0) and std::sinh(2.0) should
 * be exactly identical because the former simply calls the latter.
 * But the compiler may do constant folding because the input (2.0) is
 * constexpr. With gcc 12, it was found that in debug mode, it folds
 * std::sinh(2.0) but not element::sinh(2.0), leading to results that
 * differ by machine precision. In release mode, or with clang 16, no such
 * difference was found.
 */

TEST(ElementSinhTest, value_double) {
  EXPECT_EQ(element::sinh(0.0), 0.0);
  EXPECT_DOUBLE_EQ(element::sinh(2.0), std::sinh(2.0));
}

TEST(ElementSinhTest, value_float) {
  EXPECT_EQ(element::sinh(0.0f), 0.0f);
  EXPECT_FLOAT_EQ(element::sinh(2.0f), std::sinh(2.0f));
}

TEST(ElementCoshTest, value_double) {
  EXPECT_EQ(element::cosh(0.0), 1.0);
  EXPECT_DOUBLE_EQ(element::cosh(2.0), std::cosh(2.0));
}

TEST(ElementCoshTest, value_float) {
  EXPECT_EQ(element::cosh(0.0f), 1.0f);
  EXPECT_FLOAT_EQ(element::cosh(2.0f), std::cosh(2.0f));
}

TEST(ElementTanhTest, value_double) {
  EXPECT_EQ(element::tanh(0.0), 0.0);
  EXPECT_DOUBLE_EQ(element::tanh(2.0), std::tanh(2.0));
}

TEST(ElementTanhTest, value_float) {
  EXPECT_EQ(element::tanh(0.0f), 0.0f);
  EXPECT_FLOAT_EQ(element::tanh(2.0f), std::tanh(2.0f));
}

TEST(ElementAsinhTest, value_double) {
  EXPECT_EQ(element::asinh(0.0), 0.0);
  EXPECT_DOUBLE_EQ(element::asinh(2.0), std::asinh(2.0));
}

TEST(ElementAsinhTest, value_float) {
  EXPECT_EQ(element::asinh(0.0f), 0.0f);
  EXPECT_FLOAT_EQ(element::asinh(2.0f), std::asinh(2.0f));
}

TEST(ElementAcoshTest, value_double) {
  EXPECT_EQ(element::acosh(1.0), 0.0);
  EXPECT_DOUBLE_EQ(element::acosh(2.0), std::acosh(2.0));
}

TEST(ElementAcoshTest, value_float) {
  EXPECT_EQ(element::acosh(1.0f), 0.0f);
  EXPECT_FLOAT_EQ(element::acosh(2.0f), std::acosh(2.0f));
}

TEST(ElementAtanhTest, value_double) {
  EXPECT_EQ(element::atanh(0.0), 0.0);
  EXPECT_DOUBLE_EQ(element::atanh(0.5), std::atanh(0.5));
}

TEST(ElementAtanhTest, value_float) {
  EXPECT_EQ(element::atanh(0.0f), 0.0f);
  EXPECT_FLOAT_EQ(element::atanh(0.5f), std::atanh(0.5f));
}
