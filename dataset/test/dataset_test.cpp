// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "scipp/common/index.h"
#include "scipp/core/except.h"
#include "test_macros.h"
#include <gtest/gtest.h>

#include <numeric>
#include <set>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/reduction.h"
#include "scipp/dataset/unaligned.h"
#include "scipp/variable/operations.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::dataset;

// Any dataset functionality that is also available for Dataset(Const)View is
// to be tested in dataset_view_test.cpp, not here!

TEST(DatasetTest, construct_default) { ASSERT_NO_THROW(Dataset d); }

TEST(DatasetTest, clear) {
  DatasetFactory3D factory;
  auto dataset = factory.make();

  ASSERT_FALSE(dataset.empty());
  ASSERT_FALSE(dataset.coords().empty());
  ASSERT_FALSE(dataset.attrs().empty());
  ASSERT_FALSE(dataset.masks().empty());

  ASSERT_NO_THROW(dataset.clear());

  ASSERT_TRUE(dataset.empty());
  ASSERT_FALSE(dataset.coords().empty());
  ASSERT_FALSE(dataset.attrs().empty());
  ASSERT_FALSE(dataset.masks().empty());
}

TEST(DatasetTest, erase_non_existant) {
  Dataset d;
  ASSERT_THROW(d.erase("not an item"), except::NotFoundError);
  ASSERT_THROW(auto _ = d.extract("not an item"), except::NotFoundError);
}

TEST(DatasetTest, erase) {
  DatasetFactory3D factory;
  auto dataset = factory.make();
  ASSERT_NO_THROW(dataset.erase("data_xyz"));
  ASSERT_FALSE(dataset.contains("data_xyz"));
}

TEST(DatasetTest, extract) {
  DatasetFactory3D factory;
  auto dataset = factory.make();
  Dataset reference(dataset);

  auto array = dataset.extract("data_xyz");

  ASSERT_FALSE(dataset.contains("data_xyz"));
  EXPECT_EQ(array, reference["data_xyz"]);
  reference.erase("data_xyz");
  EXPECT_EQ(dataset, reference);
}

TEST(DatasetTest, erase_extents_rebuild) {
  Dataset d;

  d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{10}));
  ASSERT_TRUE(d.contains("a"));

  ASSERT_NO_THROW(d.erase("a"));
  ASSERT_FALSE(d.contains("a"));

  ASSERT_NO_THROW(
      d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{15})));
  ASSERT_TRUE(d.contains("a"));
}

TEST(DatasetTest, extract_extents_rebuild) {
  Dataset d;

  d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{10}));
  ASSERT_TRUE(d.contains("a"));

  ASSERT_NO_THROW(auto _ = d.extract("a"));
  ASSERT_FALSE(d.contains("a"));

  ASSERT_NO_THROW(
      d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{15})));
  ASSERT_TRUE(d.contains("a"));
}

TEST(DatasetTest, setCoord) {
  Dataset d;
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3});

  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 0);

  ASSERT_NO_THROW(d.setCoord(Dim::X, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 1);

  ASSERT_NO_THROW(d.setCoord(Dim::Y, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 2);

  ASSERT_NO_THROW(d.setCoord(Dim::X, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 2);
}

TEST(DatasetTest, setAttr) {
  Dataset d;
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3});

  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 0);

  ASSERT_NO_THROW(d.setAttr("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 1);

  ASSERT_NO_THROW(d.setAttr("b", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 2);

  ASSERT_NO_THROW(d.setAttr("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 2);
}

TEST(DatasetTest, setMask) {
  Dataset d;
  const auto var =
      makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{false, true, false});

  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.masks().size(), 0);

  ASSERT_NO_THROW(d.setMask("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.masks().size(), 1);
  ASSERT_EQ(d.masks()["a"], var);

  ASSERT_NO_THROW(d.setMask("b", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.masks().size(), 2);

  ASSERT_NO_THROW(d.setMask("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.masks().size(), 2);
}

TEST(DatasetTest, setData_with_and_without_variances) {
  Dataset d;
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3});

  ASSERT_NO_THROW(d.setData("a", var));
  ASSERT_EQ(d.size(), 1);

  ASSERT_NO_THROW(d.setData("b", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_NO_THROW(d.setData("a", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_NO_THROW(d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{3},
                                                      Values{1, 1, 1},
                                                      Variances{0, 0, 0})));
  ASSERT_EQ(d.size(), 2);
}

TEST(DatasetTest, setData_updates_dimensions) {
  const auto xy = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3});
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{2});

  Dataset d;
  d.setData("x", xy);
  d.setData("x", x);

  const auto dims = d.dimensions();
  ASSERT_TRUE(dims.find(Dim::X) != dims.end());
  // Dim::Y should no longer appear in dimensions after item "x" was replaced.
  ASSERT_TRUE(dims.find(Dim::Y) == dims.end());
}

TEST(DatasetTest, setData_clears_attributes) {
  const auto var = makeVariable<double>(Values{1});
  Dataset d;
  d.setData("x", var);
  d["x"].attrs().set("attr", var);

  EXPECT_TRUE(d["x"].attrs().contains("attr"));
  d.setData("x", var);
  EXPECT_FALSE(d["x"].attrs().contains("attr"));
}

TEST(DatasetTest, setData_keep_attributes) {
  const auto var = makeVariable<double>(Values{1});
  Dataset d;
  d.setData("x", var);
  d["x"].attrs().set("attr", var);

  EXPECT_TRUE(d["x"].attrs().contains("attr"));
  d.setData("x", var, AttrPolicy::Keep);
  EXPECT_TRUE(d["x"].attrs().contains("attr"));
}

TEST(DatasetTest, setData_with_mismatched_dims) {
  scipp::index expected_size = 2;
  const auto original =
      makeVariable<double>(Dims{Dim::X}, Shape{expected_size});
  const auto mismatched =
      makeVariable<double>(Dims{Dim::X}, Shape{expected_size + 1});
  Dataset d;

  ASSERT_NO_THROW(d.setData("a", original));
  ASSERT_THROW(d.setData("a", mismatched), except::DimensionError);
}

TEST(DatasetTest, DataArrayView_setData) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  Dataset d;
  d.setData("a", var);
  d.setData("b", var);

  EXPECT_THROW(d["a"].setData(makeVariable<double>(Dims{Dim::X}, Shape{4})),
               except::DimensionError);
  EXPECT_EQ(d["a"].data(), var);
  EXPECT_NO_THROW(d["a"].setData(var + var));
  EXPECT_EQ(d["a"].data(), var + var);
}

struct SetDataTest : public ::testing::Test {
protected:
  Variable var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  Variable y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 3});
  DataArray data{var, {{Dim::Y, var}}};
  DataArray realigned = unaligned::realign(data, {{Dim::Y, y}});
};

TEST_F(SetDataTest, DataArray_unaligned) {
  EXPECT_THROW(realigned.unaligned().setData(
                   makeVariable<double>(Dims{Dim::X}, Shape{4})),
               except::DimensionError);
  EXPECT_EQ(realigned.unaligned().data(), var);
  EXPECT_NO_THROW(realigned.unaligned().setData(var + var));
  EXPECT_EQ(realigned.unaligned().data(), var + var);
}

TEST_F(SetDataTest, DataArray_realigned) {
  // Set dense data on realigned, dropping unaligned content.
  const Variable dense_data(y.slice({Dim::Y, 0, 1}));
  EXPECT_NO_THROW(realigned.setData(dense_data));
  EXPECT_TRUE(realigned.hasData());
  EXPECT_FALSE(realigned.unaligned());
}

TEST_F(SetDataTest, DataArrayView_realigned) {
  // Set dense data on realigned via view, dropping unaligned content.
  const Variable dense_data(y.slice({Dim::Y, 0, 1}));
  EXPECT_NO_THROW(DataArrayView(realigned).setData(dense_data));
  EXPECT_TRUE(realigned.hasData());
  EXPECT_FALSE(realigned.unaligned());
}

TEST(DatasetTest, setCoord_with_name_matching_data_name) {
  Dataset d;
  d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{3}));
  d.setData("b", makeVariable<double>(Dims{Dim::X}, Shape{3}));

  // It is possible to set labels with a name matching data. However, there is
  // no special meaning attached to this. In particular it is *not* linking the
  // labels to that data item.
  ASSERT_NO_THROW(d.setCoord(Dim("a"), makeVariable<double>(Values{double{}})));
  ASSERT_EQ(d.size(), 2);
  ASSERT_EQ(d.coords().size(), 1);
  ASSERT_EQ(d["a"].coords().size(), 1);
  ASSERT_EQ(d["b"].coords().size(), 1);
}

TEST(DatasetTest, set_event_coord) {
  Dataset d;
  const auto var = makeVariable<event_list<double>>(Dims{Dim::X}, Shape{3});

  ASSERT_NO_THROW(d.coords().set(Dim::Y, var));
  ASSERT_EQ(d.size(), 0);
}

TEST(DatasetTest, iterators_return_types) {
  Dataset d;
  ASSERT_TRUE((std::is_same_v<decltype(*d.begin()), DataArrayView>));
  ASSERT_TRUE((std::is_same_v<decltype(*d.end()), DataArrayView>));
}

TEST(DatasetTest, const_iterators_return_types) {
  const Dataset d;
  ASSERT_TRUE((std::is_same_v<decltype(*d.begin()), DataArrayConstView>));
  ASSERT_TRUE((std::is_same_v<decltype(*d.end()), DataArrayConstView>));
}

TEST(DatasetTest, set_dense_data_with_events_coord) {
  auto events_variable =
      makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto dense_variable =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2l, 2l});

  Dataset a;
  a.setData("events_coord_and_val", dense_variable);
  // Events handled via dtype, not dimension, so this is valid.
  ASSERT_NO_THROW(a.coords().set(Dim::X, events_variable));

  // Setting coords first yields same response.
  Dataset b;
  b.coords().set(Dim::X, events_variable);
  ASSERT_NO_THROW(b.setData("events_coord_and_val", dense_variable));
}

TEST(DatasetTest, construct_from_view) {
  DatasetFactory3D factory;
  const auto dataset = factory.make();
  const DatasetConstView view(dataset);
  Dataset from_view(view);
  ASSERT_EQ(from_view, dataset);
}

TEST(DatasetTest, construct_from_slice) {
  DatasetFactory3D factory;
  const auto dataset = factory.make();
  const auto slice = dataset.slice({Dim::X, 1});
  Dataset from_slice(slice);
  ASSERT_EQ(from_slice, dataset.slice({Dim::X, 1}));
}

TEST(DataArrayTest, construct_from_slice) {
  DatasetFactory3D factory;
  const auto dataset = factory.make();
  const auto slice = dataset["data_xyz"].slice({Dim::X, 1});
  DataArray from_slice(slice);
  ASSERT_EQ(from_slice, dataset["data_xyz"].slice({Dim::X, 1}));
}

TEST(DatasetTest, slice_temporary) {
  DatasetFactory3D factory;
  auto dataset = factory.make().slice({Dim::X, 1});
  ASSERT_TRUE((std::is_same_v<decltype(dataset), Dataset>));
}

TEST(DatasetTest, slice_no_data) {
  Dataset d;
  d.coords().set(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{4}));
  EXPECT_TRUE(d.coords().contains(Dim::X));
  const auto slice = d.slice({Dim::X, 1, 3});
  EXPECT_TRUE(slice.coords().contains(Dim::X));
}

template <typename T> void do_test_slice_validation(const T &container) {
  EXPECT_THROW(container.slice(Slice{Dim::Y, 0, 1}), except::SliceError);
  EXPECT_THROW(container.slice(Slice{Dim::X, 0, 3}), except::SliceError);
  EXPECT_THROW(container.slice(Slice{Dim::X, -1, 0}), except::SliceError);
  EXPECT_NO_THROW(container.slice(Slice{Dim::X, 0, 1}));
}

TEST(DatasetTest, slice_validation_simple) {
  Dataset dataset;
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  dataset.setCoord(Dim::X, var);
  do_test_slice_validation(dataset);

  // Make sure correct via const proxies
  DatasetConstView constview(dataset);
  do_test_slice_validation(constview);

  // Make sure correct via proxies
  DatasetView view(dataset);
  do_test_slice_validation(view);
}

TEST(DatasetTest, slice_with_no_coords) {
  Dataset ds;
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  ds.setData("a", var);
  // No dataset coords. slicing should still work.
  auto slice = ds.slice(Slice{Dim::X, 0, 2});
  auto extents = slice["a"].data().dims()[Dim::X];
  EXPECT_EQ(extents, 2);
}

TEST(DatasetTest, slice_validation_complex) {
  Dataset ds;
  auto var1 = makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  ds.setCoord(Dim::X, var1);
  auto var2 = makeVariable<double>(Dims{Dim::Y}, Shape{4}, Values{1, 2, 3, 4});
  ds.setCoord(Dim::Y, var2);

  // Slice arguments applied in order.
  EXPECT_NO_THROW(ds.slice(Slice{Dim::X, 0, 3}).slice(Slice{Dim::X, 1, 2}));
  // Reverse order. Invalid slice creation should be caught up front.
  EXPECT_THROW(ds.slice(Slice{Dim::X, 1, 2}).slice(Slice{Dim::X, 0, 3}),
               except::SliceError);
}

TEST(DatasetTest, sum_and_mean) {
  auto ds = make_1_values_and_variances<float>("a", {Dim::X, 3}, units::one,
                                               {1, 2, 3}, {12, 15, 18});
  EXPECT_EQ(dataset::sum(ds, Dim::X)["a"].data(),
            makeVariable<float>(Values{6}, Variances{45}));
  EXPECT_EQ(dataset::sum(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
            makeVariable<float>(Values{3}, Variances{27}));

  EXPECT_EQ(dataset::mean(ds, Dim::X)["a"].data(),
            makeVariable<float>(Values{2}, Variances{5.0}));
  EXPECT_EQ(dataset::mean(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
            makeVariable<float>(Values{1.5}, Variances{6.75}));

  EXPECT_THROW(dataset::sum(make_events_2d({1, 2, 3, 4}, {0, 0}), Dim::X),
               except::TypeError);
}

TEST(DatasetTest, erase_coord) {
  DatasetFactory3D factory;
  const auto ref = factory.make();
  Dataset ds(ref);
  auto coord = Variable(ds.coords()[Dim::X]);
  ds.eraseCoord(Dim::X);
  EXPECT_FALSE(ds.coords().contains(Dim::X));
  ds.setCoord(Dim::X, coord);
  EXPECT_EQ(ref, ds);

  ds.coords().erase(Dim::X);
  EXPECT_FALSE(ds.coords().contains(Dim::X));
  ds.setCoord(Dim::X, coord);
  EXPECT_EQ(ref, ds);
}

TEST(DatasetTest, erase_labels) {
  DatasetFactory3D factory;
  const auto ref = factory.make();
  Dataset ds(ref);
  auto labels = Variable(ds.coords()[Dim("labels_x")]);
  ds.eraseCoord(Dim("labels_x"));
  EXPECT_FALSE(ds.coords().contains(Dim("labels_x")));
  ds.setCoord(Dim("labels_x"), labels);
  EXPECT_EQ(ref, ds);

  ds.coords().erase(Dim("labels_x"));
  EXPECT_FALSE(ds.coords().contains(Dim("labels_x")));
  ds.setCoord(Dim("labels_x"), labels);
  EXPECT_EQ(ref, ds);
}

TEST(DatasetTest, erase_attrs) {
  DatasetFactory3D factory;
  const auto ref = factory.make();
  Dataset ds(ref);
  auto attrs = Variable(ds.attrs()["attr_x"]);
  ds.eraseAttr("attr_x");
  EXPECT_FALSE(ds.attrs().contains("attr_x"));
  ds.setAttr("attr_x", attrs);
  EXPECT_EQ(ref, ds);

  ds.attrs().erase("attr_x");
  EXPECT_FALSE(ds.attrs().contains("attr_x"));
  ds.setAttr("attr_x", attrs);
  EXPECT_EQ(ref, ds);
}

TEST(DatasetTest, erase_masks) {
  DatasetFactory3D factory;
  const auto ref = factory.make();
  Dataset ds(ref);
  auto mask = Variable(ref.masks()["masks_x"]);
  ds.eraseMask("masks_x");
  EXPECT_FALSE(ds.masks().contains("masks_x"));
  ds.setMask("masks_x", mask);
  EXPECT_EQ(ref, ds);

  ds.masks().erase("masks_x");
  EXPECT_FALSE(ds.masks().contains("masks_x"));
  ds.setMask("masks_x", mask);
  EXPECT_EQ(ref, ds);
}

struct DatasetRenameTest : public ::testing::Test {
  DatasetRenameTest() {
    DatasetFactory3D factory(4, 5, 6, Dim::X);
    factory.seed(0);
    d = factory.make();
    original = d;
  }

protected:
  Dataset d;
  Dataset original;
};

TEST_F(DatasetRenameTest, fail_duplicate_dim) {
  ASSERT_THROW(d.rename(Dim::X, Dim::Y), except::DimensionError);
  ASSERT_EQ(d, original);
  ASSERT_THROW(d.rename(Dim::X, Dim::X), except::DimensionError);
  ASSERT_EQ(d, original);
}

TEST_F(DatasetRenameTest, back_and_forth) {
  d.rename(Dim::X, Dim::Row);
  EXPECT_NE(d, original);
  d.rename(Dim::Row, Dim::X);
  EXPECT_EQ(d, original);
}

TEST_F(DatasetRenameTest, rename) {
  d.rename(Dim::X, Dim::Row);
  DatasetFactory3D factory(4, 5, 6, Dim::Row);
  factory.seed(0);
  EXPECT_EQ(d, factory.make());
}

TEST(DatasetCoordsRealignedTest, set_erase) {
  auto d = testdata::make_dataset_realigned_x_to_y();

  // Add coord to unaligned
  EXPECT_NO_THROW(d["a"].unaligned().coords().set(
      Dim::Z, d["a"].unaligned().coords()[Dim::Y]));
  EXPECT_FALSE(d["a"].coords().contains(Dim::Z));

  // Scalar could be added to realigned -> fail
  EXPECT_THROW(d["a"].unaligned().coords().set(Dim("scalar"), 1.3 * units::K),
               except::RealignedDataError);

  // Depending only on dims of realigned -> fail
  EXPECT_THROW(d["a"].unaligned().coords().set(Dim::Y, d.coords()[Dim::Y]),
               except::RealignedDataError);

  // Depending on dim of unaligned -> works
  EXPECT_NO_THROW(d["a"].unaligned().coords().set(
      Dim::Y, d["a"].unaligned().coords()[Dim::Y] * (2.0 * units::one)));

  EXPECT_NO_THROW(d["a"].unaligned().coords().erase(Dim::Y));
  EXPECT_TRUE(d["a"].coords().contains(Dim::Y)); // bin edges still present
  EXPECT_FALSE(d["a"].unaligned().coords().contains(Dim::Y));

  // Potentially surprising but consistent behavior: "scalar" mapped from
  // realigned but cannot erase via unaligned.
  EXPECT_TRUE(d["a"].unaligned().coords().contains(Dim("scalar")));
  EXPECT_THROW(d["a"].unaligned().coords().erase(Dim("scalar")),
               except::NotFoundError);
}

TEST(DatasetMasksRealignedTest, set_erase) {
  auto d = testdata::make_dataset_realigned_x_to_y();

  // Add mask to unaligned
  EXPECT_NO_THROW(
      d["a"].unaligned().masks().set("x", d["a"].unaligned().coords()[Dim::Y]));
  EXPECT_FALSE(d["a"].masks().contains("x"));

  // Scalar could be added to realigned -> fail
  EXPECT_THROW(d["a"].unaligned().masks().set("scalar", 1.3 * units::K),
               except::RealignedDataError);

  // Depending only on dims of realigned -> fail
  EXPECT_THROW(d["a"].unaligned().masks().set("y", d.coords()[Dim::Y]),
               except::RealignedDataError);

  // Depending on dim of unaligned -> works
  EXPECT_NO_THROW(d["a"].unaligned().masks().set(
      "mask", d["a"].unaligned().coords()[Dim::Y] * (2.0 * units::one)));
  EXPECT_NO_THROW(
      d.masks().set("mask", d["a"].coords()[Dim::Y] * (2.0 * units::one)));

  EXPECT_NO_THROW(d["a"].unaligned().masks().erase("mask"));
  EXPECT_TRUE(d["a"].masks().contains("mask")); // mask of dataset still present
  EXPECT_FALSE(d["a"].unaligned().masks().contains("mask"));

  // Potentially surprising but consistent behavior: "scalar" mapped from
  // realigned but cannot erase via unaligned.
  d.masks().set("scalar", 1.3 * units::K);
  EXPECT_TRUE(d["a"].unaligned().masks().contains("scalar"));
  EXPECT_THROW(d["a"].unaligned().masks().erase("scalar"),
               except::NotFoundError);
}
