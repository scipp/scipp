// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/data_array.h"
#include "scipp/dataset/less.h"
#include "scipp/dataset/reciprocal.h"
#include "scipp/variable/less.h"
#include "scipp/variable/reciprocal.h"

#include "test_data_arrays.h"
#include "test_macros.h"

using namespace scipp;

namespace {
void check_meta(const DataArray &out, const DataArray &a) {
  EXPECT_FALSE(out.data().is_same(a.data()));
  EXPECT_EQ(out.coords(), a.coords());
  EXPECT_EQ(out.masks(), a.masks());
  // Meta data may be shallow-copied but dicts are not shared
  EXPECT_NE(&out.coords(), &a.coords());
  EXPECT_NE(&out.masks(), &a.masks());
  EXPECT_TRUE(out.coords()[Dim::X].is_same(a.coords()[Dim::X]));
  // Masks are NOT shallow-copied, just like data
  EXPECT_FALSE(out.masks()["mask"].is_same(a.masks()["mask"]));
}
} // namespace

TEST(GeneratedUnaryTest, DataArray) {
  const auto array = make_data_array_1d();
  const auto out = reciprocal(array);
  EXPECT_EQ(out.data(), reciprocal(array.data()));
  check_meta(out, array);
}

class GeneratedBinaryTest : public ::testing::Test {
protected:
  DataArray a = make_data_array_1d(1);
  DataArray b = make_data_array_1d(2);
};

TEST_F(GeneratedBinaryTest, DataArray_Variable) {
  const auto var = b.data();
  // Using `less` as an example of a generated binary function
  const auto out = less(a, var);
  EXPECT_EQ(out.data(), less(a.data(), var));
  EXPECT_FALSE(out.data().is_same(var));
  check_meta(out, a);
}

TEST_F(GeneratedBinaryTest, Variable_DataArray) {
  const auto var = b.data();
  // Using `less` as an example of a generated binary function
  const auto out = less(var, a);
  EXPECT_EQ(out.data(), less(var, a.data()));
  EXPECT_FALSE(out.data().is_same(var));
  check_meta(out, a);
}

class GeneratedBinaryDataArrayTest : public ::testing::Test {
protected:
  DataArray a = make_data_array_1d(1);
  DataArray b = make_data_array_1d(2);
  // Using `less` as an example of a generated binary function
  DataArray out = less(a, b);
};

TEST_F(GeneratedBinaryDataArrayTest, DataArray_DataArray) {
  EXPECT_FALSE(out.data().is_same(a.data()));
  EXPECT_FALSE(out.data().is_same(b.data()));
  EXPECT_EQ(out.data(), less(a.data(), b.data()));
  EXPECT_EQ(out.coords(), a.coords()); // because both inputs have same coords
  EXPECT_NE(out.masks(), a.masks());
  // Meta data may be shallow-copied but dicts are not shared
  EXPECT_NE(&out.coords(), &a.coords());
  EXPECT_NE(&out.masks(), &a.masks());
}

TEST_F(GeneratedBinaryDataArrayTest, coord_union) {
  b.coords().set(Dim("aux"), copy(b.coords()[Dim::X]));
  out = less(a, b);
  // Coords are shared
  EXPECT_TRUE(out.coords()[Dim::X].is_same(a.coords()[Dim::X]));
  EXPECT_TRUE(out.coords()[Dim("aux")].is_same(b.coords()[Dim("aux")]));
}

TEST_F(GeneratedBinaryDataArrayTest, mask_or) {
  // Masks are NOT shared
  EXPECT_FALSE(out.masks()["mask"].is_same(a.masks()["mask"]));
  EXPECT_FALSE(out.masks()["mask"].is_same(b.masks()["mask"]));
  EXPECT_EQ(out.masks()["mask"], a.masks()["mask"] | b.masks()["mask"]);
  // masks only in one input are deep-copied
  EXPECT_FALSE(out.masks()["mask1"].is_same(a.masks()["mask1"]));
  EXPECT_FALSE(out.masks()["mask2"].is_same(b.masks()["mask2"]));
  EXPECT_EQ(out.masks()["mask1"], a.masks()["mask1"]);
  EXPECT_EQ(out.masks()["mask2"], b.masks()["mask2"]);
}

TEST_F(GeneratedBinaryDataArrayTest, mask_is_deep_copied_even_if_same) {
  EXPECT_FALSE(less(a, a).masks()["mask"].is_same(a.masks()["mask"]));
}

TEST_F(GeneratedBinaryDataArrayTest, non_bool_masks_with_same_names) {
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0.1, 0.2});
  auto coord =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m, Values{1, 2});
  auto mask = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0.1, 0.1});
  a = DataArray(data, {{Dim::X, coord}}, {{"mask", mask}});
  ASSERT_THROW_DISCARD(less(a, a), except::TypeError);
  ASSERT_THROW_DISCARD(a += a, except::TypeError);
}
