// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <numeric>

#include "scipp/dataset/astype.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "test_macros.h"

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

DataArray make_example_dataarray(const std::string_view mask_name,
                                 Dim mask_dim) {
  std::vector<double> data_values(4);
  std::iota(data_values.begin(), data_values.end(), 0);

  auto data = makeVariable<double>(Dimensions{{Dim::X, 2}, {Dim::Y, 2}},
                                   Values(data_values));
  auto mask =
      makeVariable<bool>(Dimensions{mask_dim, 2}, Values({true, false}));
  auto da = DataArray(data);
  da.masks().set(std::string(mask_name), mask);
  return da;
}

Dataset make_example_dataset(const std::string_view xmask_name = "mask_x",
                             const std::string_view ymask_name = "mask_y") {
  auto a = make_example_dataarray(xmask_name, Dim::X);
  auto b = make_example_dataarray(ymask_name, Dim::Y);
  return Dataset({{"a", a}, {"b", b}});
}

TEST_F(SetSliceTest, set_dataarray_slice_when_metadata_missing) {
  auto ds = make_example_dataset("mask_x", "mask_y");
  auto original = copy(ds);
  auto point = ds["a"].slice({Dim::X, 1}).slice({Dim::Y, 1}); // Only has mask_x
  EXPECT_THROW_DISCARD(ds["b"].setSlice(Slice{Dim::Y, 0}, point),
                       except::NotFoundError);
  EXPECT_THROW_DISCARD(ds["a"].setSlice(Slice{Dim::Y, 0}, point),
                       except::DimensionError);
  // We test for a partially-applied modification as a result of an aborted
  // transaction. Check that "a" is NOT getting modified before operation falls
  // over on "b", which has no mask_x.
  EXPECT_EQ(original, ds); // Failed op should have no effect
}

TEST_F(SetSliceTest, set_dataarray_slice_with_forbidden_broadcast_of_mask) {
  auto ds = make_example_dataset("mask", "mask");
  auto original = copy(ds);
  auto point = ds["a"].slice({Dim::X, 1}).slice({Dim::Y, 1});
  EXPECT_THROW_DISCARD(ds.setSlice(Slice{Dim::Y, 0}, point),
                       except::DimensionError);
  // We test for a partially-applied modification as a result of an aborted
  // transaction. Check that "a" is NOT getting modified before operation falls
  // over on "b" as would involve broadcasting mask in non-slice dimension. Mask
  // should be read-only.
  EXPECT_EQ(original, ds); // Failed op should have no effect
}
TEST_F(SetSliceTest, set_dataset_slice_with_forbidden_broadcast_of_mask) {
  auto ds = make_example_dataset("mask_x", "mask_y");
  auto original = copy(ds);
  auto point = ds.slice({Dim::X, 1}).slice({Dim::Y, 1});
  EXPECT_THROW_DISCARD(ds.setSlice(Slice{Dim::Y, 0}, point),
                       except::DimensionError);
  // We test for a partially-applied modification as a result of an aborted
  // transaction. Check that "a" is NOT getting modified before operation falls
  // over on "b".
  EXPECT_EQ(original, ds); // Failed op should have no effect

  EXPECT_THROW_DISCARD(ds.setSlice(Slice{Dim::X, 0}, point),
                       except::DimensionError);
  EXPECT_EQ(original, ds); // Failed op should have no effect
}

TEST_F(SetSliceTest, set_dataarray_slice_with_different_data_units_forbidden) {
  auto da = make_example_dataarray("mask", Dim::X);
  auto original = copy(da);
  auto other = copy(da);
  other.setUnit(scipp::sc_units::K); // slice to use now had different unit
  other = other.slice({Dim::X, 1}).slice({Dim::Y, 1});
  EXPECT_THROW_DISCARD(da.setSlice(Slice{Dim::X, 0}, other), except::UnitError);
  // We test for a partially-applied modification as a result of an aborted
  // transaction. Check that masks are NOT getting modified before operation
  // falls over on unit mismatch for data
  EXPECT_EQ(original, da); // Failed op should have no effect
}

TEST_F(SetSliceTest, set_dataarray_slice_with_variances_forbidden) {
  auto da = make_example_dataarray("mask", Dim::X);
  auto original = copy(da);
  auto other = copy(da);
  other.data().setVariances(other.data()); // Slice now has variances
  other = other.slice({Dim::X, 1}).slice({Dim::Y, 1});
  EXPECT_THROW_DISCARD(da.setSlice(Slice{Dim::X, 0}, other),
                       except::VariancesError);
  // We test for a partially-applied modification as a result of an aborted
  // transaction. Check that masks are NOT getting modified before operation
  // falls over on variances for data
  EXPECT_EQ(original, da); // Failed op should have no effect
}

TEST_F(SetSliceTest, set_dataarray_slice_with_different_dtype_forbidden) {
  auto da = make_example_dataarray("mask", Dim::X);
  auto original = copy(da);
  auto other = astype(da, dtype<float>);
  other = other.slice({Dim::X, 1}).slice({Dim::Y, 1});
  EXPECT_THROW_DISCARD(da.setSlice(Slice{Dim::X, 0}, other), except::TypeError);
  // We test for a partially-applied modification as a result of an aborted
  // transaction. Check that masks are NOT getting modified before operation
  // falls over on differing type for data
  EXPECT_EQ(original, da); // Failed op should have no effect
}
