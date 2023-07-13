// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/arithmetic.h"

#include "dataset_test_common.h"
#include "test_macros.h"

using namespace scipp;

class DataArrayArithmeticCoordTest : public ::testing::Test {
protected:
  DataArrayArithmeticCoordTest()
      : aligned_1(makeVariable<double>(Dims{dim}, Shape{len}, Values{1, 2, 3})),
        aligned_2(makeVariable<double>(Dims{dim}, Shape{len}, Values{4, 5, 6})),
        unaligned_1(aligned_1), unaligned_2(aligned_2),
        data(makeVariable<double>(Dims{dim}, Shape{len}, Values{10, 20, 30})) {
    unaligned_1.set_aligned(false);
    unaligned_2.set_aligned(false);
  }

  Dim dim = Dim::X;
  scipp::index len = 3;
  Variable aligned_1;
  Variable aligned_2;
  Variable unaligned_1;
  Variable unaligned_2;
  Variable data;
};

TEST_F(DataArrayArithmeticCoordTest, aligned_aligned_match) {
  const DataArray a(data, {{dim, aligned_1}});
  const DataArray b(data, {{dim, aligned_1}});
  const auto res = a + b;
  EXPECT_EQ(size(res.coords()), 1);
  EXPECT_EQ(res.coords()[dim], aligned_1);
  EXPECT_TRUE(res.coords()[dim].is_aligned());
}

TEST_F(DataArrayArithmeticCoordTest, aligned_aligned_mismatch) {
  const DataArray a(data, {{dim, aligned_1}});
  const DataArray b(data, {{dim, aligned_2}});
  EXPECT_THROW_DISCARD(a + b, except::CoordMismatchError);
  EXPECT_THROW_DISCARD(b + a, except::CoordMismatchError);
}

TEST_F(DataArrayArithmeticCoordTest, aligned_missing) {
  const DataArray a(data, {{dim, aligned_1}});
  const DataArray b(data, {});

  const auto res_1 = a + b;
  EXPECT_EQ(size(res_1.coords()), 1);
  EXPECT_EQ(res_1.coords()[dim], aligned_1);
  EXPECT_TRUE(res_1.coords()[dim].is_aligned());

  const auto res_2 = b + a;
  EXPECT_EQ(size(res_2.coords()), 1);
  EXPECT_EQ(res_2.coords()[dim], aligned_1);
  EXPECT_TRUE(res_2.coords()[dim].is_aligned());
}

TEST_F(DataArrayArithmeticCoordTest, aligned_unaligned_match) {
  const DataArray a(data, {{dim, aligned_1}});
  const DataArray b(data, {{dim, unaligned_1}});

  const auto res_1 = a + b;
  EXPECT_EQ(size(res_1.coords()), 1);
  EXPECT_EQ(res_1.coords()[dim], aligned_1);
  EXPECT_TRUE(res_1.coords()[dim].is_aligned());
  EXPECT_TRUE(aligned_1.is_aligned());
  EXPECT_FALSE(unaligned_1.is_aligned());

  const auto res_2 = b + a;
  EXPECT_EQ(size(res_2.coords()), 1);
  EXPECT_EQ(res_2.coords()[dim], aligned_1);
  EXPECT_TRUE(res_2.coords()[dim].is_aligned());
  EXPECT_TRUE(aligned_1.is_aligned());
  EXPECT_FALSE(unaligned_1.is_aligned());
}

TEST_F(DataArrayArithmeticCoordTest, aligned_unaligned_mismatch) {
  const DataArray a(data, {{dim, aligned_1}});
  const DataArray b(data, {{dim, unaligned_2}});

  const auto res_1 = a + b;
  EXPECT_EQ(size(res_1.coords()), 1);
  EXPECT_EQ(res_1.coords()[dim], aligned_1);
  EXPECT_TRUE(res_1.coords()[dim].is_aligned());
  EXPECT_TRUE(aligned_1.is_aligned());
  EXPECT_FALSE(unaligned_2.is_aligned());

  const auto res_2 = b + a;
  EXPECT_EQ(size(res_2.coords()), 1);
  EXPECT_EQ(res_2.coords()[dim], aligned_1);
  EXPECT_TRUE(res_2.coords()[dim].is_aligned());
  EXPECT_TRUE(aligned_1.is_aligned());
  EXPECT_FALSE(unaligned_2.is_aligned());
}

TEST_F(DataArrayArithmeticCoordTest, unaligned_unaligned_match) {
  const DataArray a(data, {{dim, unaligned_1}});
  const DataArray b(data, {{dim, unaligned_1}});

  const auto res = a + b;
  EXPECT_EQ(size(res.coords()), 1);
  EXPECT_EQ(res.coords()[dim], unaligned_1);
  EXPECT_FALSE(res.coords()[dim].is_aligned());
}

TEST_F(DataArrayArithmeticCoordTest, unaligned_unaligned_mismatch) {
  const DataArray a(data, {{dim, unaligned_1}});
  const DataArray b(data, {{dim, unaligned_2}});

  const auto res_1 = a + b;
  EXPECT_TRUE(res_1.coords().empty());

  const auto res_2 = b + a;
  EXPECT_TRUE(res_2.coords().empty());
}

// This is needed to ensure (a + b) + c == a + (b + c)
// e.g. if a, b, c all have an unaligned coord x, all with different values.
TEST_F(DataArrayArithmeticCoordTest, unaligned_missing) {
  const DataArray a(data, {{dim, unaligned_1}});
  const DataArray b(data, {});

  const auto res_1 = a + b;
  EXPECT_TRUE(res_1.coords().empty());

  const auto res_2 = b + a;
  EXPECT_TRUE(res_2.coords().empty());
}

TEST(DataArrayArithmeticTest, produces_correct_data) {
  const DataArray a(
      makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{1, 2}),
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{3, 4})}});
  const DataArray b(
      makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{10, 20}),
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{3, 4})}});
  EXPECT_EQ((a + b).data(), a.data() + b.data());
  EXPECT_EQ((a - b).data(), a.data() - b.data());
}
