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

  ASSERT_NO_THROW(dataset.clear());

  ASSERT_TRUE(dataset.empty());
  ASSERT_FALSE(dataset.coords().empty());
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

  auto ptr = dataset["data_xyz"].values<double>().data();
  auto array = dataset.extract("data_xyz");
  EXPECT_EQ(array.values<double>().data(), ptr);

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

TEST(DatasetTest, setCoord_grow) {
  const auto var3 = makeVariable<double>(Dims{Dim::X}, Shape{3});
  const auto var4 = makeVariable<double>(Dims{Dim::X}, Shape{4});
  Dataset d;
  ASSERT_NO_THROW(d.setCoord(Dim::X, var3));
  ASSERT_NO_THROW(d.setCoord(Dim::Y, var4));
}

TEST(DatasetTest, setCoord_shrink) {
  const auto var3 = makeVariable<double>(Dims{Dim::X}, Shape{3});
  const auto var4 = makeVariable<double>(Dims{Dim::X}, Shape{4});
  Dataset d;
  ASSERT_NO_THROW(d.setCoord(Dim::X, var4));
  ASSERT_NO_THROW(d.setCoord(Dim::Y, var3));
}

TEST(DatasetTest, setCoord_fail_events_on_edges) {
  const auto events = makeVariable<event_list<double>>(Dims{Dim::X}, Shape{4});
  Dataset d;
  d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{3}));
  ASSERT_THROW(d.setCoord(Dim::Y, events), except::DimensionError);
}

TEST(DatasetTest, set_item_mask) {
  Dataset d;
  d.setData("x", makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  d.setData("scalar", 1.2 * units::one);
  const auto var =
      makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{false, true, false});
  d["x"].masks().set("unaligned", var);
  EXPECT_TRUE(d["x"].masks().contains("unaligned"));
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
  d["x"].coords().set(Dim("attr"), var);

  EXPECT_TRUE(d["x"].coords().contains(Dim("attr")));
  d.setData("x", var);
  EXPECT_FALSE(d["x"].coords().contains(Dim("attr")));
}

TEST(DatasetTest, setData_keep_attributes) {
  const auto var = makeVariable<double>(Values{1});
  Dataset d;
  d.setData("x", var);
  d["x"].coords().set(Dim("attr"), var);

  EXPECT_TRUE(d["x"].coords().contains(Dim("attr")));
  d.setData("x", var, AttrPolicy::Keep);
  EXPECT_TRUE(d["x"].coords().contains(Dim("attr")));
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

TEST(DatasetTest, size_in_memory_test) {
  Dataset d;
  const auto data =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto mask = makeVariable<double>(Dims{Dim::X}, Shape{3});
  const auto coords = makeVariable<double>(Dims{Dim::X}, Shape{3});

  d.setData("a", data);
  EXPECT_EQ(d.sizeInMemory(), sizeof(double) * 3);

  d.setCoord(Dim::X, coords);
  EXPECT_EQ(d.sizeInMemory(), sizeof(double) * 6);

  d["a"].masks().set("unaligned", mask);
  EXPECT_EQ(d.sizeInMemory(), sizeof(double) * 9);
}

struct SetDataTest : public ::testing::Test {
protected:
  Variable var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  Variable y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 3});
  DataArray data{var, {{Dim::Y, var}}};
};

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

TEST(DatasetTest, extract_coord) {
  DatasetFactory3D factory;
  const auto ref = factory.make();
  Dataset ds(ref);
  auto coord = Variable(ds.coords()[Dim::X]);
  auto ptr = ds.coords()[Dim::X].values<double>().data();
  auto var = ds.extractCoord(Dim::X);
  EXPECT_EQ(var.values<double>().data(), ptr);
  EXPECT_FALSE(ds.coords().contains(Dim::X));
  ds.setCoord(Dim::X, coord);
  EXPECT_EQ(ref, ds);

  ds.coords().erase(Dim::X);
  EXPECT_FALSE(ds.coords().contains(Dim::X));
  ds.setCoord(Dim::X, coord);
  EXPECT_EQ(ref, ds);
}

TEST(DatasetTest, erase_item_coord_cannot_erase_coord) {
  DatasetFactory3D factory;
  const auto ref = factory.make();
  Dataset ds(ref);
  auto coord = Variable(ds.coords()[Dim::X]);
  ASSERT_TRUE(ds.contains("data_x"));
  EXPECT_THROW(ds["data_x"].coords().erase(Dim::X), except::NotFoundError);
}

TEST(DatasetTest, extract_labels) {
  DatasetFactory3D factory;
  const auto ref = factory.make();
  Dataset ds(ref);
  auto labels = Variable(ds.coords()[Dim("labels_x")]);
  ds.extractCoord(Dim("labels_x"));
  EXPECT_FALSE(ds.coords().contains(Dim("labels_x")));
  ds.setCoord(Dim("labels_x"), labels);
  EXPECT_EQ(ref, ds);

  ds.coords().erase(Dim("labels_x"));
  EXPECT_FALSE(ds.coords().contains(Dim("labels_x")));
  ds.setCoord(Dim("labels_x"), labels);
  EXPECT_EQ(ref, ds);
}

TEST(DatasetTest, set_erase_item_attr) {
  DatasetFactory3D factory;
  auto ds = factory.make();
  const auto attr = makeVariable<double>(Values{1.0});
  ds["data_x"].coords().set(Dim("item-attr"), attr);
  EXPECT_TRUE(ds["data_x"].coords().contains(Dim("item-attr")));
  ds["data_x"].coords().erase(Dim("item-attr"));
  EXPECT_FALSE(ds["data_x"].coords().contains(Dim("item-attr")));
}

TEST(DatasetTest, set_erase_item_mask) {
  DatasetFactory3D factory;
  auto ds = factory.make();
  const auto mask = makeVariable<double>(Values{true});
  ds["data_x"].masks().set("item-mask", mask);
  EXPECT_TRUE(ds["data_x"].masks().contains("item-mask"));
  ds["data_x"].masks().erase("item-mask");
  EXPECT_FALSE(ds["data_x"].masks().contains("item-mask"));
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
