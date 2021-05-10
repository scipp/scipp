// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/data_array.h"
#include "scipp/dataset/less.h"
#include "scipp/dataset/reciprocal.h"
#include "scipp/variable/less.h"
#include "scipp/variable/reciprocal.h"

#include "test_data_arrays.h"

using namespace scipp;

TEST(GeneratedUnaryTest, DataArray) {
  const auto array = make_data_array_1d();
  const auto out = reciprocal(array);
  EXPECT_FALSE(out.data().is_same(array.data()));
  EXPECT_EQ(out.data(), reciprocal(array.data()));
  EXPECT_EQ(out.coords(), array.coords());
  EXPECT_EQ(out.masks(), array.masks());
  EXPECT_EQ(out.attrs(), array.attrs());
  // Meta data is shallow-copied but dicts are not shared
  EXPECT_NE(&out.coords(), &array.coords());
  EXPECT_NE(&out.masks(), &array.masks());
  EXPECT_NE(&out.attrs(), &array.attrs());
  EXPECT_TRUE(out.coords()[Dim::X].is_same(array.coords()[Dim::X]));
  EXPECT_TRUE(out.masks()["mask"].is_same(array.masks()["mask"]));
  EXPECT_TRUE(out.attrs()[Dim("attr")].is_same(array.attrs()[Dim("attr")]));
}

class GeneratedBinaryTest : public ::testing::Test {
protected:
  DataArray a = make_data_array_1d(1);
  DataArray b = make_data_array_1d(2);
};

namespace {
void check_meta(const DataArray &out, const DataArray &a) {
  EXPECT_FALSE(out.data().is_same(a.data()));
  EXPECT_EQ(out.coords(), a.coords());
  EXPECT_EQ(out.masks(), a.masks());
  EXPECT_EQ(out.attrs(), a.attrs());
  // Meta data may be shallow-copied but dicts are not shared
  EXPECT_NE(&out.coords(), &a.coords());
  EXPECT_NE(&out.masks(), &a.masks());
  EXPECT_NE(&out.attrs(), &a.attrs());
  EXPECT_TRUE(out.coords()[Dim::X].is_same(a.coords()[Dim::X]));
  // Masks are NOT shallow-copied, just like data
  EXPECT_FALSE(out.masks()["mask"].is_same(a.masks()["mask"]));
  EXPECT_TRUE(out.attrs()[Dim("attr")].is_same(a.attrs()[Dim("attr")]));
}
} // namespace

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

TEST_F(GeneratedBinaryTest, DataArray_DataArray) {
  // Using `less` as an example of a generated binary function
  const auto out = less(a, b);
  EXPECT_FALSE(out.data().is_same(a.data()));
  EXPECT_FALSE(out.data().is_same(b.data()));
  EXPECT_EQ(out.data(), less(a.data(), b.data()));
  EXPECT_EQ(out.coords(), a.coords());
  EXPECT_NE(out.masks(), a.masks()); // union is not same
  EXPECT_NE(out.attrs(), a.attrs()); // intersection
  // Meta data is shallow-copied but dicts are not shared
  EXPECT_NE(&out.coords(), &a.coords());
  EXPECT_NE(&out.masks(), &a.masks());
  EXPECT_NE(&out.attrs(), &a.attrs());
  EXPECT_TRUE(out.coords()[Dim::X].is_same(a.coords()[Dim::X]));
  // mask is OR of inputs, even if same
  EXPECT_FALSE(out.masks()["mask"].is_same(a.masks()["mask"]));
  // mask only in one input are copied
  EXPECT_FALSE(out.masks()["mask1"].is_same(a.masks()["mask1"]));
  EXPECT_FALSE(out.masks()["mask2"].is_same(b.masks()["mask2"]));
  EXPECT_TRUE(out.attrs()[Dim("attr")].is_same(a.attrs()[Dim("attr")]));
}
