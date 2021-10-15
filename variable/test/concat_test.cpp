// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/variable/shape.h"

using namespace scipp;

class ConcatTest : public ::testing::Test {
protected:
  Variable base = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                       Values{1, 2, 3, 4});
};

TEST_F(ConcatTest, new_dim) {
  EXPECT_EQ(
      concat(std::vector{base.slice({Dim::X, 0}), base.slice({Dim::X, 1})},
             Dim::X),
      base);
}

TEST(ConcatenateTest, concatenate) {
  Dimensions dims(Dim::X, 1);
  auto a = makeVariable<double>(Dimensions(dims), Values{1.0});
  auto b = makeVariable<double>(Dimensions(dims), Values{2.0});
  a.setUnit(units::m);
  b.setUnit(units::m);
  auto ab = concatenate(a, b, Dim::X);
  ASSERT_EQ(ab.dims().volume(), 2);
  EXPECT_EQ(ab.unit(), units::m);
  const auto &data = ab.values<double>();
  EXPECT_EQ(data[0], 1.0);
  EXPECT_EQ(data[1], 2.0);
  auto ba = concatenate(b, a, Dim::X);
  const auto abba = concatenate(ab, ba, Dim::Y);
  ASSERT_EQ(abba.dims().volume(), 4);
  EXPECT_EQ(abba.dims().shape().size(), 2);
  const auto &data2 = abba.values<double>();
  EXPECT_EQ(data2[0], 1.0);
  EXPECT_EQ(data2[1], 2.0);
  EXPECT_EQ(data2[2], 2.0);
  EXPECT_EQ(data2[3], 1.0);
  const auto ababbaba = concatenate(abba, abba, Dim::X);
  ASSERT_EQ(ababbaba.dims().volume(), 8);
  const auto &data3 = ababbaba.values<double>();
  EXPECT_EQ(data3[0], 1.0);
  EXPECT_EQ(data3[1], 2.0);
  EXPECT_EQ(data3[2], 1.0);
  EXPECT_EQ(data3[3], 2.0);
  EXPECT_EQ(data3[4], 2.0);
  EXPECT_EQ(data3[5], 1.0);
  EXPECT_EQ(data3[6], 2.0);
  EXPECT_EQ(data3[7], 1.0);
  const auto abbaabba = concatenate(abba, abba, Dim::Y);
  ASSERT_EQ(abbaabba.dims().volume(), 8);
  const auto &data4 = abbaabba.values<double>();
  EXPECT_EQ(data4[0], 1.0);
  EXPECT_EQ(data4[1], 2.0);
  EXPECT_EQ(data4[2], 2.0);
  EXPECT_EQ(data4[3], 1.0);
  EXPECT_EQ(data4[4], 1.0);
  EXPECT_EQ(data4[5], 2.0);
  EXPECT_EQ(data4[6], 2.0);
  EXPECT_EQ(data4[7], 1.0);
}

TEST(ConcatenateTest, concatenate_volume_with_slice) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1.0});
  auto aa = concatenate(a, a, Dim::X);
  EXPECT_NO_THROW_DISCARD(concatenate(aa, a, Dim::X));
}

TEST(ConcatenateTest, concatenate_slice_with_volume) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1.0});
  auto aa = concatenate(a, a, Dim::X);
  EXPECT_NO_THROW_DISCARD(concatenate(a, aa, Dim::X));
}

TEST(ConcatenateTest, concatenate_fail) {
  Dimensions dims(Dim::X, 1);
  auto a = makeVariable<double>(Dimensions(dims), Values{1.0});
  auto b = makeVariable<double>(Dimensions(dims), Values{2.0});
  auto c = makeVariable<float>(Dimensions(dims), Values{2.0});
  EXPECT_THROW_MSG_DISCARD(
      concatenate(a, c, Dim::X), std::runtime_error,
      "Cannot concatenate Variables: Data types do not match.");
  auto aa = concatenate(a, a, Dim::X);
  EXPECT_THROW_MSG_DISCARD(
      concatenate(a, aa, Dim::Y), std::runtime_error,
      "Cannot concatenate Variables: Dimension extents do not match.");
}

TEST(ConcatenateTest, concatenate_unit_fail) {
  Dimensions dims(Dim::X, 1);
  auto a = makeVariable<double>(Dimensions(dims), Values{1.0});
  auto b = copy(a);
  EXPECT_NO_THROW_DISCARD(concatenate(a, b, Dim::X));
  a.setUnit(units::m);
  EXPECT_THROW_MSG_DISCARD(concatenate(a, b, Dim::X), std::runtime_error,
                           "Cannot concatenate Variables: Units do not match.");
  b.setUnit(units::m);
  EXPECT_NO_THROW_DISCARD(concatenate(a, b, Dim::X));
}

TEST(ConcatenateTest, concatenate_from_slices_with_broadcast) {
  auto input_v = {0.0, 0.1, 0.2, 0.3};
  auto var = makeVariable<double>(Dimensions{Dim::X, 4}, Values(input_v),
                                  Variances(input_v));
  auto out = concatenate(var.slice(Slice(Dim::X, 1, 4)),
                         var.slice(Slice(Dim::X, 0, 3)), Dim::Y);
  auto expected = {0.1, 0.2, 0.3, 0.0, 0.1, 0.2};
  EXPECT_EQ(out, makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                      Values(expected), Variances(expected)));
}
