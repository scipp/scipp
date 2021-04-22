// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

using namespace scipp;
using namespace scipp::dataset;

struct SetSliceTest : public ::testing::Test {
  SetSliceTest() {}

protected:
  Dimensions dims{Dim::X, 3};
  Variable data = makeVariable<double>(dims, Values{1, 2, 3});
  Variable x = makeVariable<double>(dims, Values{1, 1, 3});
  Variable mask = makeVariable<bool>(dims, Values{true, false, true});
  DataArray array{data, {{Dim::X, copy(x)}}, {{"mask", copy(mask)}}};
};

TEST_F(SetSliceTest, self) {
  const auto original = copy(array);
  EXPECT_EQ(array.setSlice({Dim::X, 0, 3}, array), original);
}

TEST_F(SetSliceTest, copy_slice) {
  ASSERT_NO_THROW(array.slice({Dim::X, 0}));
  ASSERT_NO_THROW(array.slice({Dim::X, 0}).masks());
  EXPECT_THROW(array.slice({Dim::X, 0})
                   .masks()
                   .set("abc", makeVariable<bool>(Values{false})),
               std::runtime_error);
}

TEST_F(SetSliceTest, coord_fail) {
  const auto original = copy(array);
  EXPECT_THROW(array.setSlice({Dim::X, 0, 1}, array.slice({Dim::X, 2, 3})),
               except::CoordMismatchError);
  EXPECT_EQ(array, original);
}

TEST_F(SetSliceTest, mask_propagation) {
  const auto original = copy(array);
  // Mask values get copied
  array.setSlice({Dim::X, 0}, original.slice({Dim::X, 1}));
  EXPECT_EQ(array.masks()["mask"],
            makeVariable<bool>(dims, Values{false, false, true}));
  array.setSlice({Dim::X, 0}, original.slice({Dim::X, 2}));
  EXPECT_EQ(array.masks()["mask"],
            makeVariable<bool>(dims, Values{true, false, true}));
  // Mask not in source is preserved unchanged
  array.masks().set("other", copy(mask));
  array.setSlice({Dim::X, 0}, original.slice({Dim::X, 1}));
  EXPECT_EQ(array.masks()["other"], mask);
}

TEST_F(SetSliceTest, new_meta_data_cannot_be_added) {
  const auto original = copy(array);
  auto extra_mask = copy(array.slice({Dim::X, 1}));
  extra_mask.masks().set("extra", copy(mask.slice({Dim::X, 1})));
  EXPECT_THROW(array.setSlice({Dim::X, 0}, extra_mask), except::NotFoundError);
  EXPECT_EQ(array, original);
}

TEST_F(SetSliceTest, new_meta_data_cannot_be_added_arithmetic) {
  const auto original = copy(array);
  auto extra_mask = copy(array.slice({Dim::X, 1}));
  extra_mask.masks().set("extra", copy(mask.slice({Dim::X, 1})));
  EXPECT_THROW(array.slice({Dim::X, 0}) += extra_mask, except::NotFoundError);
  EXPECT_EQ(array, original);
}

TEST_F(SetSliceTest, lower_dimensional_mask_cannot_be_overridden) {
  auto other = copy(array.slice({Dim::X, 1}));
  array.masks().set("scalar", makeVariable<bool>(Values{true}));
  EXPECT_NO_THROW(array.setSlice({Dim::X, 0}, other));
  other.masks().set("scalar", makeVariable<bool>(Values{true}));
  EXPECT_NO_THROW(array.setSlice({Dim::X, 0}, other)); // ok, no change
  other.masks().set("scalar", makeVariable<bool>(Values{false}));
  // Setting a slice must not change mask values of unrelated data points
  const auto original = copy(array);
  EXPECT_THROW(array.setSlice({Dim::X, 0}, other), except::DimensionError);
  EXPECT_EQ(array, original);
}

TEST_F(SetSliceTest, lower_dimensional_mask_cannot_be_overridden_arithmetic) {
  auto other = copy(array.slice({Dim::X, 0}));
  array.masks().set("scalar", makeVariable<bool>(Values{false}));
  const auto original = copy(array);
  EXPECT_NO_THROW(array.slice({Dim::X, 1}) += other);
  other.masks().set("scalar", makeVariable<bool>(Values{false}));
  EXPECT_NO_THROW(array.slice({Dim::X, 1}) += other); // ok, no change
  other.masks().set("scalar", makeVariable<bool>(Values{true}));
  // Setting a slice must not change mask values of unrelated data points
  array = copy(original);
  EXPECT_THROW(array.slice({Dim::X, 1}) += other, except::DimensionError);
  EXPECT_EQ(array, original);
}
