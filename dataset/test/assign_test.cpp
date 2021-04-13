// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

using namespace scipp;
using namespace scipp::dataset;

struct AssignTest : public ::testing::Test {
  AssignTest() {}

protected:
  Dimensions dims{Dim::X, 3};
  Variable data = makeVariable<double>(dims, Values{1, 2, 3});
  Variable x = makeVariable<double>(dims, Values{1, 1, 3});
  Variable mask = makeVariable<bool>(dims, Values{true, false, true});
  DataArray array{data, {{Dim::X, copy(x)}}, {{"mask", copy(mask)}}};
};

TEST_F(AssignTest, self) {
  const auto original = copy(array);
  // EXPECT_EQ(array.setSlice(array), original);
}

TEST_F(AssignTest, coord_fail) {
  const auto original = copy(array);
  // EXPECT_THROW(array.setSlice(array.slice({Dim::X, 0, 1})),
  //             except::CoordMismatchError);
  // EXPECT_EQ(array, original);
  EXPECT_THROW(array.setSlice({Dim::X, 0, 1}, array.slice({Dim::X, 2, 3})),
               except::CoordMismatchError);
  EXPECT_EQ(array, original);
}

TEST_F(AssignTest, mask_propagation) {
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
  // Extra mask is added
  auto extra_mask = copy(array);
  extra_mask.masks().set("extra", copy(mask));
  // EXPECT_NO_THROW(array.setSlice(extra_mask.slice({Dim::X, 1})));
  // EXPECT_TRUE(array.masks().contains("extra"));
  // Extra masks added to mask dict of slice => silently dropped
  extra_mask.masks().set("dropped", copy(mask));
  EXPECT_THROW(array.setSlice({Dim::X, 0}, extra_mask.slice({Dim::X, 1})),
               except::NotFoundError);
  EXPECT_FALSE(array.masks().contains("dropped"));
}

TEST_F(AssignTest, lower_dimensional_mask_cannot_be_overridden) {
  auto other = copy(array.slice({Dim::X, 1}));
  array.masks().set("scalar", makeVariable<bool>(Values{true}));
  EXPECT_NO_THROW(array.setSlice({Dim::X, 0}, other));
  other.masks().set("scalar", makeVariable<bool>(Values{false}));
  // Setting a slice must not change mask values of unrelated data points
  EXPECT_THROW(array.setSlice({Dim::X, 0}, other), except::DimensionError);
}
