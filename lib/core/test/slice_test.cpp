// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/except.h"
#include "scipp/core/slice.h"

using namespace scipp;

TEST(SliceTest, test_default_construct) {
  Slice slice;
  EXPECT_EQ(slice.dim(), Dim::None);
}

TEST(SliceTest, test_construction) {
  Slice point(Dim::X, 0);
  EXPECT_EQ(point.dim(), Dim::X);
  EXPECT_EQ(point.begin(), 0);
  EXPECT_EQ(point.end(), -1);
  EXPECT_TRUE(!point.isRange());

  Slice range(Dim::X, 0, 1);
  EXPECT_EQ(range.dim(), Dim::X);
  EXPECT_EQ(range.begin(), 0);
  EXPECT_EQ(range.end(), 1);
  EXPECT_TRUE(range.isRange());
}

TEST(SliceTest, test_equals) {
  Slice ref{Dim::X, 1, 2};

  EXPECT_EQ(ref, ref);
  EXPECT_EQ(ref, (Slice{Dim::X, 1, 2}));
  EXPECT_NE(ref, (Slice{Dim::Y, 1, 2}));
  EXPECT_NE(ref, (Slice{Dim::X, 0, 2}));
  EXPECT_NE(ref, (Slice{Dim::X, 1, 3}));
}

TEST(SliceTest, test_assignment) {
  Slice a{Dim::X, 1, 2};
  Slice b{Dim::Y, 2, 3};
  a = b;
  EXPECT_EQ(a, b);
}

TEST(SliceTest, test_begin_valid) {
  EXPECT_THROW((Slice{Dim::X, -1 /*invalid begin index*/, 1}),
               except::SliceError);
}

TEST(SliceTest, test_end_valid) {
  EXPECT_THROW((Slice{
                   Dim::X, 2, 1 /*invalid end index*/
               }),
               except::SliceError);
}

TEST(SliceTest, stride_is_1_if_not_specified) {
  EXPECT_EQ(Slice().stride(), 1);
  EXPECT_EQ(Slice(Dim::X, 2).stride(), 1);
  EXPECT_EQ(Slice(Dim::X, 2, 4).stride(), 1);
}

TEST(SliceTest, positive_stride_can_be_set) {
  Slice slice(Dim::X, 1, 10, 3);
  EXPECT_EQ(slice.stride(), 3);
}

TEST(SliceTest, negative_stride_throws_SliceError) {
  // Not implemented yet, this is not based on a requirement
  EXPECT_THROW(Slice(Dim::X, 1, 10, -1), except::SliceError);
}

// DISABLED since support not implemented yet
TEST(SliceTest, DISABLED_negative_stride_can_be_set) {
  Slice slice(Dim::X, 1, 10, -3);
  EXPECT_EQ(slice.stride(), -3);
}

TEST(SliceTest, zero_stride_throws_SliceError) {
  EXPECT_THROW(Slice(Dim::X, 1, 10, 0), except::SliceError);
}
