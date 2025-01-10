// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/core/except.h"
#include "scipp/variable/variable.h"

using namespace scipp;

auto make_range() {
  return makeVariable<double>(Dims{Dim::X}, Shape{10},
                              Values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
}

TEST(VariableSliceTest, full_slice_with_stride_1_gives_original) {
  const auto var = make_range();
  EXPECT_EQ(var.slice({Dim::X, 0, 10}), var);
  EXPECT_EQ(var.slice({Dim::X, 0, 10, 1}), var);
}

TEST(VariableSliceTest, stride_2_gives_every_other) {
  const auto var = make_range();
  EXPECT_EQ(
      var.slice({Dim::X, 0, 10, 2}),
      makeVariable<double>(Dims{Dim::X}, Shape{5}, Values{1, 3, 5, 7, 9}));
  EXPECT_EQ(
      var.slice({Dim::X, 1, 10, 2}),
      makeVariable<double>(Dims{Dim::X}, Shape{5}, Values{2, 4, 6, 8, 10}));
  EXPECT_EQ(var.slice({Dim::X, 2, 10, 2}),
            makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{3, 5, 7, 9}));
}

TEST(VariableSliceTest, stride_3_gives_every_third) {
  const auto var = make_range();
  EXPECT_EQ(var.slice({Dim::X, 0, 10, 3}),
            makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 4, 7, 10}));
  EXPECT_EQ(var.slice({Dim::X, 1, 10, 3}),
            makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2, 5, 8}));
  EXPECT_EQ(var.slice({Dim::X, 2, 10, 3}),
            makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{3, 6, 9}));
  EXPECT_EQ(var.slice({Dim::X, 3, 10, 3}),
            makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{4, 7, 10}));
}

TEST(VariableSliceTest, negative_stride_throws) {
  // Currently class Slice cannot be created with negative stride. This is a
  // sanity check since Variable::slice needs modifications if class Slice
  // started to support this. See DISABLED tests below.
  const auto var = make_range();
  ASSERT_ANY_THROW_DISCARD(var.slice({Dim::X, 0, 10, -1}));
}

TEST(VariableSliceTest,
     DISABLED_negative_stride_1_with_positive_range_is_empty) {
  const auto var = make_range();
  EXPECT_EQ(var.slice({Dim::X, 0, 10, -1}),
            makeVariable<double>(Dims{Dim::X}, Shape{0}));
}

TEST(VariableSliceTest,
     DISABLED_negative_stride_1_with_negative_range_reverses) {
  const auto var = make_range();
  // Note the missing 1
  EXPECT_EQ(var.slice({Dim::X, 10, 0, -1}),
            makeVariable<double>(Dims{Dim::X}, Shape{9},
                                 Values{10, 9, 8, 7, 6, 5, 4, 3, 2}));
}
