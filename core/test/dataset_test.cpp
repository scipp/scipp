// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include <numeric>
#include <set>

#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::core;

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

TEST(DatasetTest, erase_single_non_existant) {
  Dataset d;
  ASSERT_THROW(d.erase("not an item"), except::DatasetError);
}

TEST(DatasetTest, erase_single) {
  DatasetFactory3D factory;
  auto dataset = factory.make();
  ASSERT_NO_THROW(dataset.erase("data_xyz"));
  ASSERT_FALSE(dataset.contains("data_xyz"));
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

TEST(DatasetTest, set_unaligned_coord) {
  Dataset d;
  const auto var = makeVariable<event_list<double>>(Dims{Dim::X}, Shape{3});
  DatasetAxis y;
  y.unaligned().set("a", var);

  ASSERT_NO_THROW(d.coords().set(Dim::Y, y));
  ASSERT_EQ(d.size(), 0);
}

TEST(DatasetTest, setSparseLabels) {
  Dataset d;
  const auto sparse = makeVariable<event_list<double>>(Dims{}, Shape{});
  DatasetAxis x;
  x.unaligned().set("a", sparse);
  d.coords().set(Dim::X, x);
  DatasetAxis label;
  label.unaligned().set("a", sparse);

  ASSERT_NO_THROW(d.coords().set(Dim("label"), label));
  ASSERT_EQ(d.size(), 1);
  ASSERT_NO_THROW(d["a"]);
  ASSERT_EQ(d["a"].coords().size(), 2);
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

TEST(DatasetTest, set_dense_data_with_sparse_coord) {
  auto sparse_variable =
      makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto dense_variable =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2l, 2l});

  Dataset a;
  a.setData("sparse_coord_and_val", dense_variable);
  DatasetAxis x;
  x.unaligned().set("sparse_coord_and_val", sparse_variable);
  ASSERT_THROW(a.coords().set(Dim::X, x), except::DimensionError);

  // Setting coords first yields same response.
  Dataset b;
  b.coords().set(Dim::X, x);
  ASSERT_THROW(b.setData("sparse_coord_and_val", dense_variable),
               except::DimensionError);
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

TEST(DatasetTest, slice_temporary) {
  DatasetFactory3D factory;
  auto dataset = factory.make().slice({Dim::X, 1});
  ASSERT_TRUE((std::is_same_v<decltype(dataset), Dataset>));
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
  EXPECT_NO_THROW(ds.slice(Slice{Dim::X, 0, 3}, Slice{Dim::X, 1, 2}));
  // Reverse order. Invalid slice creation should be caught up front.
  EXPECT_THROW(ds.slice(Slice{Dim::X, 1, 2}, Slice{Dim::X, 0, 3}),
               except::SliceError);
}

TEST(DatasetTest, sum_and_mean) {
  auto ds = make_1_values_and_variances<float>(
      "a", {Dim::X, 3}, units::dimensionless, {1, 2, 3}, {12, 15, 18});
  EXPECT_EQ(core::sum(ds, Dim::X)["a"].data(),
            makeVariable<float>(Values{6}, Variances{45}));
  EXPECT_EQ(core::sum(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
            makeVariable<float>(Values{3}, Variances{27}));

  EXPECT_EQ(core::mean(ds, Dim::X)["a"].data(),
            makeVariable<float>(Values{2}, Variances{5.0}));
  EXPECT_EQ(core::mean(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
            makeVariable<float>(Values{1.5}, Variances{6.75}));

  EXPECT_THROW(core::sum(make_sparse_2d({1, 2, 3, 4}, {0, 0}), Dim::X),
               except::DimensionError);
}

TEST(DatasetTest, erase_coord) {
  DatasetFactory3D factory;
  const auto ref = factory.make();
  Dataset ds(ref);
  auto coord = Variable(ds.coords()[Dim::X].data());
  ds.eraseCoord(Dim::X);
  EXPECT_FALSE(ds.coords().contains(Dim::X));
  ds.setCoord(Dim::X, coord);
  EXPECT_EQ(ref, ds);

  ds.coords().erase(Dim::X);
  EXPECT_FALSE(ds.coords().contains(Dim::X));
  ds.setCoord(Dim::X, coord);
  EXPECT_EQ(ref, ds);

  const auto var = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{4});
  DatasetAxis x;
  x.unaligned().set("newCoord", var);
  ds.coords().set(Dim::X, x);
  EXPECT_TRUE(ds["newCoord"].coords().contains(Dim::X));
  ds.coords()[Dim::X].unaligned().erase("newCoord");
  EXPECT_EQ(ref, ds);

  ds.coords().set(Dim::X, x);
  ds.setData("newCoord", makeVariable<event_list<double>>(Dims{}, Shape{}));
  EXPECT_TRUE(ds["newCoord"].coords().contains(Dim::X));
  EXPECT_THROW(ds["newCoord"].coords().erase(Dim::Z), except::SparseDataError);
  ds["newCoord"].coords().erase(Dim::X);
  EXPECT_FALSE(ds["newCoord"].coords().contains(Dim::X));
}

TEST(DatasetTest, erase_labels) {
  DatasetFactory3D factory;
  const auto ref = factory.make();
  Dataset ds(ref);
  auto labels = Variable(ds.coords()[Dim("labels_x")].data());
  ds.eraseCoord(Dim("labels_x"));
  EXPECT_FALSE(ds.coords().contains(Dim("labels_x")));
  ds.setCoord(Dim("labels_x"), labels);
  EXPECT_EQ(ref, ds);

  ds.coords().erase(Dim("labels_x"));
  EXPECT_FALSE(ds.coords().contains(Dim("labels_x")));
  ds.setCoord(Dim("labels_x"), labels);
  EXPECT_EQ(ref, ds);

  const auto var = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{5});
  DatasetAxis x;
  x.unaligned().set("newCoord", var);
  ds.coords().set(Dim::X, x);
  DatasetAxis labels_sparse;
  labels_sparse.unaligned().set("newCoord", var);
  ds.coords().set(Dim("labels_sparse"), labels_sparse);
  EXPECT_TRUE(ds["newCoord"].coords().contains(Dim("labels_sparse")));
  ds.coords()[Dim("labels_sparse")].unaligned().erase("newCoord");
  EXPECT_FALSE(ds["newCoord"].coords().contains(Dim("labels_sparse")));

  ds.coords().set(Dim("labels_sparse"), labels_sparse);
  EXPECT_TRUE(ds["newCoord"].coords().contains(Dim("labels_sparse")));
  ds["newCoord"].coords().erase(Dim("labels_sparse"));
  EXPECT_FALSE(ds["newCoord"].coords().contains(Dim("labels_sparse")));
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
