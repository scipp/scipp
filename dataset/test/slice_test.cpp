// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <numeric>

#include "dataset_test_common.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/core/slice.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/arithmetic.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(SliceTest, test_construction) {
  Slice point(Dim::X, 0);
  EXPECT_EQ(point.dim(), Dim::X);
  EXPECT_EQ(point.begin(), 0);
  EXPECT_EQ(point.end(), -1);
  EXPECT_TRUE(!point.isRange());

  Slice range(Dim::X, 0, 1);
  EXPECT_EQ(range.dim(), Dim::X);
  EXPECT_EQ(range.begin(), 0);
  EXPECT_EQ(range.end(), 1);
  EXPECT_TRUE(range.isRange());
}

TEST(SliceTest, test_equals) {
  Slice ref{Dim::X, 1, 2};

  EXPECT_EQ(ref, ref);
  EXPECT_EQ(ref, (Slice{Dim::X, 1, 2}));
  EXPECT_NE(ref, (Slice{Dim::Y, 1, 2}));
  EXPECT_NE(ref, (Slice{Dim::X, 0, 2}));
  EXPECT_NE(ref, (Slice{Dim::X, 1, 3}));
}

TEST(SliceTest, test_assignment) {
  Slice a{Dim::X, 1, 2};
  Slice b{Dim::Y, 2, 3};
  a = b;
  EXPECT_EQ(a, b);
}

TEST(SliceTest, test_begin_valid) {
  EXPECT_THROW((Slice{Dim::X, -1 /*invalid begin index*/, 1}),
               except::SliceError);
}

TEST(SliceTest, test_end_valid) {
  EXPECT_THROW((Slice{
                   Dim::X, 2, 1 /*invalid end index*/
               }),
               except::SliceError);
}

class Dataset3DTest : public ::testing::Test {
protected:
  Dataset3DTest() : dataset(factory.make()) {}

  Dataset
  datasetWithEdges(const std::initializer_list<units::Dim::Id> &edgeDims) {
    auto d = dataset;
    for (const auto dim : edgeDims) {
      auto dims = dataset.coords()[dim].dims();
      dims.resize(dim, dims[dim] + 1);
      d.setCoord(dim, makeRandom(dims));
    }
    return d;
  }

  DatasetFactory3D factory;
  Dataset dataset;
};

TEST_F(Dataset3DTest, dimension_extent_check_replace_with_edge_coord) {
  auto edge_coord = dataset;
  ASSERT_NO_THROW(edge_coord.setCoord(Dim::X, makeRandom({Dim::X, 5})));
  ASSERT_NE(edge_coord["data_xyz"], dataset["data_xyz"]);
  // Cannot incrementally grow.
  ASSERT_ANY_THROW(edge_coord.setCoord(Dim::X, makeRandom({Dim::X, 6})));
  // Minor implementation shortcoming: Currently we cannot go back to
  // non-edges.
  ASSERT_ANY_THROW(edge_coord.setCoord(Dim::X, makeRandom({Dim::X, 4})));
}

TEST_F(Dataset3DTest,
       dimension_extent_check_prevents_non_edge_coord_with_edge_data) {
  // If we reduce the X extent to 3 we would have data defined at the edges,
  // but
  // the coord is not. This is forbidden.
  ASSERT_ANY_THROW(dataset.setCoord(Dim::X, makeRandom({Dim::X, 3})));
  // We *can* set data with X extent 3. The X coord is now bin edges, and
  // other
  // data is defined on the edges.
  ASSERT_NO_THROW(dataset.setData("non_edge_data", makeRandom({Dim::X, 3})));
  // Now the X extent of the dataset is 3, but since we have data on the edges
  // we still cannot change the coord to non-edges.
  ASSERT_ANY_THROW(dataset.setCoord(Dim::X, makeRandom({Dim::X, 3})));
}

TEST_F(Dataset3DTest,
       dimension_extent_check_prevents_setting_edge_data_without_edge_coord) {
  ASSERT_ANY_THROW(dataset.setData("edge_data", makeRandom({Dim::X, 5})));
  ASSERT_NO_THROW(dataset.setCoord(Dim::X, makeRandom({Dim::X, 5})));
  ASSERT_NO_THROW(dataset.setData("edge_data", makeRandom({Dim::X, 5})));
}

TEST_F(Dataset3DTest, dimension_extent_check_non_coord_dimension_fail) {
  // This is the Y coordinate but has extra extent in X.
  ASSERT_ANY_THROW(
      dataset.setCoord(Dim::Y, makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
}

TEST_F(Dataset3DTest, dimension_extent_check_labels_dimension_fail) {
  // We cannot have labels on edges unless the coords are also edges. Note the
  // slight inconsistency though: Labels are typically though of as being for a
  // particular dimension (the inner one), but we can have labels on edges also
  // for the other dimensions (x in this case), just like data.
  ASSERT_ANY_THROW(dataset.setCoord(Dim("bad_labels"),
                                    makeRandom({{Dim::X, 4}, {Dim::Y, 6}})));
  ASSERT_ANY_THROW(dataset.setCoord(Dim("bad_labels"),
                                    makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
  dataset.setCoord(Dim::Y, makeRandom({{Dim::X, 4}, {Dim::Y, 6}}));
  ASSERT_ANY_THROW(dataset.setCoord(Dim("bad_labels"),
                                    makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
  dataset.setCoord(Dim::X, makeRandom({Dim::X, 5}));
  ASSERT_NO_THROW(dataset.setCoord(Dim("good_labels"),
                                   makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
  ASSERT_NO_THROW(dataset.setCoord(Dim("good_labels"),
                                   makeRandom({{Dim::X, 5}, {Dim::Y, 6}})));
  ASSERT_NO_THROW(dataset.setCoord(Dim("good_labels"),
                                   makeRandom({{Dim::X, 4}, {Dim::Y, 6}})));
  ASSERT_NO_THROW(dataset.setCoord(Dim("good_labels"),
                                   makeRandom({{Dim::X, 4}, {Dim::Y, 5}})));
}

class Dataset3DTest_slice_x : public Dataset3DTest,
                              public ::testing::WithParamInterface<int> {
protected:
  Dataset reference(const scipp::index pos) {
    Dataset d;
    d.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
    d.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
    d.setCoord(Dim::Z, dataset.coords()[Dim::Z].slice({Dim::X, pos}));
    d.setCoord(Dim("labels_xy"),
               dataset.coords()[Dim("labels_xy")].slice({Dim::X, pos}));
    d.setCoord(Dim("labels_z"), dataset.coords()[Dim("labels_z")]);
    d.setMask("masks_xy", dataset.masks()["masks_xy"].slice({Dim::X, pos}));
    d.setMask("masks_z", dataset.masks()["masks_z"]);
    d.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
    d.setData("values_x", dataset["values_x"].data().slice({Dim::X, pos}));
    d.setData("data_x", dataset["data_x"].data().slice({Dim::X, pos}));
    d.setData("data_xy", dataset["data_xy"].data().slice({Dim::X, pos}));
    d.setData("data_zyx", dataset["data_zyx"].data().slice({Dim::X, pos}));
    d.setData("data_xyz", dataset["data_xyz"].data().slice({Dim::X, pos}));
    return d;
  }
};
class Dataset3DTest_slice_y : public Dataset3DTest,
                              public ::testing::WithParamInterface<int> {};
class Dataset3DTest_slice_z : public Dataset3DTest,
                              public ::testing::WithParamInterface<int> {};
class Dataset3DTest_slice_events : public Dataset3DTest,
                                   public ::testing::WithParamInterface<int> {};

class Dataset3DTest_slice_range_x : public Dataset3DTest,
                                    public ::testing::WithParamInterface<
                                        std::pair<scipp::index, scipp::index>> {
};
class Dataset3DTest_slice_range_y : public Dataset3DTest,
                                    public ::testing::WithParamInterface<
                                        std::pair<scipp::index, scipp::index>> {
protected:
  Dataset reference(const scipp::index begin, const scipp::index end) {
    Dataset d;
    d.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
    d.setCoord(Dim::X, dataset.coords()[Dim::X]);
    d.setCoord(Dim::Y, dataset.coords()[Dim::Y].slice({Dim::Y, begin, end}));
    d.setCoord(Dim::Z, dataset.coords()[Dim::Z].slice({Dim::Y, begin, end}));
    d.setCoord(Dim("labels_x"), dataset.coords()[Dim("labels_x")]);
    d.setCoord(Dim("labels_xy"),
               dataset.coords()[Dim("labels_xy")].slice({Dim::Y, begin, end}));
    d.setCoord(Dim("labels_z"), dataset.coords()[Dim("labels_z")]);

    d.setMask("masks_x", dataset.masks()["masks_x"]);
    d.setMask("masks_xy",
              dataset.masks()["masks_xy"].slice({Dim::Y, begin, end}));
    d.setMask("masks_z", dataset.masks()["masks_z"]);

    d.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
    d.setAttr("attr_x", dataset.attrs()["attr_x"]);
    d.setData("data_xy", dataset["data_xy"].data().slice({Dim::Y, begin, end}));
    d.setData("data_zyx",
              dataset["data_zyx"].data().slice({Dim::Y, begin, end}));
    d.setData("data_xyz",
              dataset["data_xyz"].data().slice({Dim::Y, begin, end}));
    return d;
  }
};
class Dataset3DTest_slice_range_z : public Dataset3DTest,
                                    public ::testing::WithParamInterface<
                                        std::pair<scipp::index, scipp::index>> {
protected:
  Dataset reference(const scipp::index begin, const scipp::index end) {
    Dataset d;
    d.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
    d.setCoord(Dim::X, dataset.coords()[Dim::X]);
    d.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
    d.setCoord(Dim::Z, dataset.coords()[Dim::Z].slice({Dim::Z, begin, end}));
    d.setCoord(Dim("labels_x"), dataset.coords()[Dim("labels_x")]);
    d.setCoord(Dim("labels_xy"), dataset.coords()[Dim("labels_xy")]);
    d.setCoord(Dim("labels_z"),
               dataset.coords()[Dim("labels_z")].slice({Dim::Z, begin, end}));
    d.setMask("masks_x", dataset.masks()["masks_x"]);
    d.setMask("masks_xy", dataset.masks()["masks_xy"]);
    d.setMask("masks_z",
              dataset.masks()["masks_z"].slice({Dim::Z, begin, end}));
    d.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
    d.setAttr("attr_x", dataset.attrs()["attr_x"]);
    d.setData("data_zyx",
              dataset["data_zyx"].data().slice({Dim::Z, begin, end}));
    d.setData("data_xyz",
              dataset["data_xyz"].data().slice({Dim::Z, begin, end}));
    return d;
  }
};

/// Return all valid ranges (iterator pairs) for a container of given length.
template <int max> constexpr auto valid_ranges() {
  using scipp::index;
  const auto size = max + 1;
  std::array<std::pair<index, index>, ((size * size + size) / 2) - 1> pairs;
  index i = 0;
  for (index first = 0; first < max; ++first)
    for (index second = first + 0; second <= max; ++second) {
      pairs[i].first = first;
      pairs[i].second = second;
      ++i;
    }
  return pairs;
}

static auto ranges_x = valid_ranges<4>();
static auto ranges_y = valid_ranges<5>();
static auto ranges_z = valid_ranges<6>();

INSTANTIATE_TEST_SUITE_P(AllPositions, Dataset3DTest_slice_x,
                         ::testing::Range(0, 4));
INSTANTIATE_TEST_SUITE_P(AllPositions, Dataset3DTest_slice_y,
                         ::testing::Range(0, 5));
INSTANTIATE_TEST_SUITE_P(AllPositions, Dataset3DTest_slice_z,
                         ::testing::Range(0, 6));

INSTANTIATE_TEST_SUITE_P(NonEmptyRanges, Dataset3DTest_slice_range_x,
                         ::testing::ValuesIn(ranges_x));
INSTANTIATE_TEST_SUITE_P(NonEmptyRanges, Dataset3DTest_slice_range_y,
                         ::testing::ValuesIn(ranges_y));
INSTANTIATE_TEST_SUITE_P(NonEmptyRanges, Dataset3DTest_slice_range_z,
                         ::testing::ValuesIn(ranges_z));
INSTANTIATE_TEST_SUITE_P(AllPositions, Dataset3DTest_slice_events,
                         ::testing::Range(0, 2));

TEST_P(Dataset3DTest_slice_x, slice) {
  const auto pos = GetParam();
  auto expected = reference(pos);
  // Non-range slice converts coord to attr
  for (const auto &name :
       {"values_x", "data_x", "data_xy", "data_zyx", "data_xyz"})
    for (const auto &attr : {"x", "labels_x"})
      expected[name].attrs().set(
          attr, dataset.coords()[Dim(attr)].slice({Dim::X, pos}));
  EXPECT_EQ(dataset.slice({Dim::X, pos}), expected);
}

TEST_P(Dataset3DTest_slice_events, slice) {
  Dataset ds;
  const auto pos = GetParam();
  auto var =
      makeVariable<event_list<double>>(Dims{Dim::X, Dim::Y}, Shape{2, 2});
  var.values<event_list<double>>()[0] = {1, 2, 3};
  var.values<event_list<double>>()[1] = {4, 5, 6};
  var.values<event_list<double>>()[2] = {7};
  var.values<event_list<double>>()[3] = {8, 9};

  ds.setData("xyz_data", var);
  ds.setCoord(Dim::X,
              makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0, 1}));
  ds.setCoord(Dim::Y,
              makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{0, 1}));

  auto sliced = ds.slice({Dim::X, pos});
  auto data = sliced["xyz_data"].data().values<event_list<double>>();
  EXPECT_EQ(data.size(), 2);
  event_list<double> expected = var.values<event_list<double>>()[pos * 2];
  EXPECT_EQ(data[0], expected);
  expected = var.values<event_list<double>>()[pos * 2 + 1];
  EXPECT_EQ(data[1], expected);
}

TEST_P(Dataset3DTest_slice_x, slice_bin_edges) {
  const auto pos = GetParam();
  auto datasetWithEdges = dataset;
  datasetWithEdges.setCoord(Dim::X, makeRandom({Dim::X, 5}));
  auto expected = reference(pos);
  // Non-range slice converts coord to attr
  for (const auto &name :
       {"values_x", "data_x", "data_xy", "data_zyx", "data_xyz"}) {
    expected[name].attrs().set(
        "labels_x",
        datasetWithEdges.coords()[Dim("labels_x")].slice({Dim::X, pos}));
    expected[name].attrs().set(
        "x", datasetWithEdges.coords()[Dim("x")].slice({Dim::X, pos, pos + 2}));
  }
  EXPECT_EQ(datasetWithEdges.slice({Dim::X, pos}), expected);
}

TEST_P(Dataset3DTest_slice_y, slice) {
  const auto pos = GetParam();
  Dataset reference;
  reference.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
  reference.setCoord(Dim::X, dataset.coords()[Dim::X]);
  reference.setCoord(Dim::Z, dataset.coords()[Dim::Z].slice({Dim::Y, pos}));
  reference.setCoord(Dim("labels_x"), dataset.coords()[Dim("labels_x")]);
  reference.setCoord(Dim("labels_z"), dataset.coords()[Dim("labels_z")]);
  reference.setMask("masks_x", dataset.masks()["masks_x"]);
  reference.setMask("masks_z", dataset.masks()["masks_z"]);
  reference.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
  reference.setAttr("attr_x", dataset.attrs()["attr_x"]);
  reference.setData("data_xy", dataset["data_xy"].data().slice({Dim::Y, pos}));
  reference.setData("data_zyx",
                    dataset["data_zyx"].data().slice({Dim::Y, pos}));
  reference.setData("data_xyz",
                    dataset["data_xyz"].data().slice({Dim::Y, pos}));
  for (const auto &name : {"data_xy", "data_zyx", "data_xyz"})
    for (const auto &attr : {"y", "labels_xy"})
      reference[name].attrs().set(
          attr, dataset.coords()[Dim(attr)].slice({Dim::Y, pos}));

  EXPECT_EQ(dataset.slice({Dim::Y, pos}), reference);
}

TEST_P(Dataset3DTest_slice_z, slice) {
  const auto pos = GetParam();
  Dataset reference;
  reference.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
  reference.setCoord(Dim::X, dataset.coords()[Dim::X]);
  reference.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  reference.setCoord(Dim("labels_x"), dataset.coords()[Dim("labels_x")]);
  reference.setCoord(Dim("labels_xy"), dataset.coords()[Dim("labels_xy")]);
  reference.setMask("masks_x", dataset.masks()["masks_x"]);
  reference.setMask("masks_xy", dataset.masks()["masks_xy"]);
  reference.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
  reference.setAttr("attr_x", dataset.attrs()["attr_x"]);
  reference.setData("data_zyx",
                    dataset["data_zyx"].data().slice({Dim::Z, pos}));
  reference.setData("data_xyz",
                    dataset["data_xyz"].data().slice({Dim::Z, pos}));
  for (const auto &name : {"data_zyx", "data_xyz"})
    for (const auto &attr : {"z", "labels_z"})
      reference[name].attrs().set(
          attr, dataset.coords()[Dim(attr)].slice({Dim::Z, pos}));

  EXPECT_EQ(dataset.slice({Dim::Z, pos}), reference);
}

TEST_P(Dataset3DTest_slice_range_x, slice) {
  const auto [begin, end] = GetParam();
  Dataset reference;
  reference.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
  reference.setCoord(Dim::X,
                     dataset.coords()[Dim::X].slice({Dim::X, begin, end}));
  reference.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  reference.setCoord(Dim::Z,
                     dataset.coords()[Dim::Z].slice({Dim::X, begin, end}));
  reference.setCoord(Dim("labels_x"), dataset.coords()[Dim("labels_x")].slice(
                                          {Dim::X, begin, end}));
  reference.setCoord(Dim("labels_xy"), dataset.coords()[Dim("labels_xy")].slice(
                                           {Dim::X, begin, end}));
  reference.setCoord(Dim("labels_z"), dataset.coords()[Dim("labels_z")]);
  reference.setMask("masks_x",
                    dataset.masks()["masks_x"].slice({Dim::X, begin, end}));
  reference.setMask("masks_xy",
                    dataset.masks()["masks_xy"].slice({Dim::X, begin, end}));
  reference.setMask("masks_z", dataset.masks()["masks_z"]);
  reference.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
  reference.setAttr("attr_x",
                    dataset.attrs()["attr_x"].slice({Dim::X, begin, end}));
  reference.setData("values_x",
                    dataset["values_x"].data().slice({Dim::X, begin, end}));
  reference.setData("data_x",
                    dataset["data_x"].data().slice({Dim::X, begin, end}));
  reference.setData("data_xy",
                    dataset["data_xy"].data().slice({Dim::X, begin, end}));
  reference.setData("data_zyx",
                    dataset["data_zyx"].data().slice({Dim::X, begin, end}));
  reference.setData("data_xyz",
                    dataset["data_xyz"].data().slice({Dim::X, begin, end}));

  EXPECT_EQ(dataset.slice({Dim::X, begin, end}), reference);
}

TEST_P(Dataset3DTest_slice_range_y, slice) {
  const auto [begin, end] = GetParam();
  EXPECT_EQ(dataset.slice({Dim::Y, begin, end}), reference(begin, end));
}

TEST_P(Dataset3DTest_slice_range_y, slice_with_edges) {
  const auto [begin, end] = GetParam();
  auto datasetWithEdges = dataset;
  const auto yEdges = makeRandom({Dim::Y, 6});
  datasetWithEdges.setCoord(Dim::Y, yEdges);
  auto referenceWithEdges = reference(begin, end);
  // Is this the correct behavior for edges also in case the range is empty?
  referenceWithEdges.setCoord(Dim::Y, yEdges.slice({Dim::Y, begin, end + 1}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::Y, begin, end}), referenceWithEdges);
}

TEST_P(Dataset3DTest_slice_range_y, slice_with_z_edges) {
  const auto [begin, end] = GetParam();
  auto datasetWithEdges = dataset;
  const auto zEdges = makeRandom({{Dim::X, 4}, {Dim::Y, 5}, {Dim::Z, 7}});
  datasetWithEdges.setCoord(Dim::Z, zEdges);
  auto referenceWithEdges = reference(begin, end);
  referenceWithEdges.setCoord(Dim::Z, zEdges.slice({Dim::Y, begin, end}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::Y, begin, end}), referenceWithEdges);
}

TEST_P(Dataset3DTest_slice_range_z, slice) {
  const auto [begin, end] = GetParam();
  EXPECT_EQ(dataset.slice({Dim::Z, begin, end}), reference(begin, end));
}

TEST_P(Dataset3DTest_slice_range_z, slice_with_edges) {
  const auto [begin, end] = GetParam();
  auto datasetWithEdges = dataset;
  const auto zEdges = makeRandom({{Dim::X, 4}, {Dim::Y, 5}, {Dim::Z, 7}});
  datasetWithEdges.setCoord(Dim::Z, zEdges);
  auto referenceWithEdges = reference(begin, end);
  referenceWithEdges.setCoord(Dim::Z, zEdges.slice({Dim::Z, begin, end + 1}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::Z, begin, end}), referenceWithEdges);
}

TEST_F(Dataset3DTest, nested_slice) {
  for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
    EXPECT_EQ(dataset.slice({dim, 1, 3}).slice({dim, 1}),
              dataset.slice({dim, 2}));
  }
}

TEST_F(Dataset3DTest, nested_slice_range) {
  for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
    EXPECT_EQ(dataset.slice({dim, 1, 3}).slice({dim, 0, 2}),
              dataset.slice({dim, 1, 3}));
    EXPECT_EQ(dataset.slice({dim, 1, 3}).slice({dim, 1, 2}),
              dataset.slice({dim, 2, 3}));
  }
}

TEST_F(Dataset3DTest, nested_slice_range_bin_edges) {
  auto datasetWithEdges = dataset;
  datasetWithEdges.setCoord(Dim::X, makeRandom({Dim::X, 5}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::X, 1, 3}).slice({Dim::X, 0, 2}),
            datasetWithEdges.slice({Dim::X, 1, 3}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::X, 1, 3}).slice({Dim::X, 1, 2}),
            datasetWithEdges.slice({Dim::X, 2, 3}));
}

TEST_F(Dataset3DTest, commutative_slice) {
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}).slice({Dim::Y, 2}),
            dataset.slice({Dim::Y, 2}).slice({Dim::X, 1, 3}));
  EXPECT_EQ(
      dataset.slice({Dim::X, 1, 3}).slice({Dim::Y, 2}).slice({Dim::Z, 3, 4}),
      dataset.slice({Dim::Y, 2}).slice({Dim::Z, 3, 4}).slice({Dim::X, 1, 3}));
  EXPECT_EQ(
      dataset.slice({Dim::X, 1, 3}).slice({Dim::Y, 2}).slice({Dim::Z, 3, 4}),
      dataset.slice({Dim::Z, 3, 4}).slice({Dim::Y, 2}).slice({Dim::X, 1, 3}));
  EXPECT_EQ(
      dataset.slice({Dim::X, 1, 3}).slice({Dim::Y, 2}).slice({Dim::Z, 3, 4}),
      dataset.slice({Dim::Z, 3, 4}).slice({Dim::X, 1, 3}).slice({Dim::Y, 2}));
}

TEST_F(Dataset3DTest, commutative_slice_range) {
  const auto &d = dataset;
  EXPECT_EQ(d.slice({Dim::X, 1, 3}).slice({Dim::Y, 2, 4}),
            d.slice({Dim::Y, 2, 4}).slice({Dim::X, 1, 3}));
  EXPECT_EQ(
      d.slice({Dim::X, 1, 3}).slice({Dim::Y, 2, 4}).slice({Dim::Z, 3, 4}),
      d.slice({Dim::Y, 2, 4}).slice({Dim::Z, 3, 4}).slice({Dim::X, 1, 3}));
  EXPECT_EQ(
      d.slice({Dim::X, 1, 3}).slice({Dim::Y, 2, 4}).slice({Dim::Z, 3, 4}),
      d.slice({Dim::Z, 3, 4}).slice({Dim::Y, 2, 4}).slice({Dim::X, 1, 3}));
  EXPECT_EQ(
      d.slice({Dim::X, 1, 3}).slice({Dim::Y, 2, 4}).slice({Dim::Z, 3, 4}),
      d.slice({Dim::Z, 3, 4}).slice({Dim::X, 1, 3}).slice({Dim::Y, 2, 4}));
}

using DataArrayViewTypes = ::testing::Types<DataArrayView, DataArrayConstView>;

template <typename T> class DataArrayView3DTest : public Dataset3DTest {
protected:
  using dataset_type = std::conditional_t<std::is_same_v<T, DataArrayView>,
                                          Dataset, const Dataset>;

  dataset_type &dataset() { return Dataset3DTest::dataset; }
};

TYPED_TEST_SUITE(DataArrayView3DTest, DataArrayViewTypes);

// We have tests that ensure that Dataset::slice is correct (and its item access
// returns the correct data), so we rely on that for verifying the results of
// slicing DataArrayView.
TYPED_TEST(DataArrayView3DTest, slice_single) {
  auto &d = TestFixture::dataset();
  for (const auto &item : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i}), d.slice({dim, i})[item.name()]);
        EXPECT_ANY_THROW(item.slice({dim, item.dims()[dim]}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0}));
      }
    }
  }
}

TYPED_TEST(DataArrayView3DTest, slice_length_0) {
  auto &d = TestFixture::dataset();
  for (const auto &item : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1, -1}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i, i + 0}),
                    d.slice({dim, i, i + 0})[item.name()]);
        // 0 thickness beyond end is ok
        EXPECT_NO_THROW(
            item.slice({dim, item.dims()[dim], item.dims()[dim] + 0}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0, 0}));
      }
    }
  }
}

TYPED_TEST(DataArrayView3DTest, slice_length_1) {
  auto &d = TestFixture::dataset();
  for (const auto &item : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1, 0}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i, i + 1}),
                    d.slice({dim, i, i + 1})[item.name()]);
        EXPECT_ANY_THROW(
            item.slice({dim, item.dims()[dim], item.dims()[dim] + 1}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0, 0}));
      }
    }
  }
}

TYPED_TEST(DataArrayView3DTest, slice) {
  auto &d = TestFixture::dataset();
  for (const auto &item : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1, 1}));
        for (scipp::index i = 0; i < item.dims()[dim] - 1; ++i)
          EXPECT_EQ(item.slice({dim, i, i + 2}),
                    d.slice({dim, i, i + 2})[item.name()]);
        EXPECT_ANY_THROW(
            item.slice({dim, item.dims()[dim], item.dims()[dim] + 2}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0, 2}));
      }
    }
  }
}

TYPED_TEST(DataArrayView3DTest, slice_slice_range) {
  auto &d = TestFixture::dataset();
  const auto slice = d.slice({Dim::X, 2, 4});
  // Slice view created from DatasetView as opposed to directly from Dataset.
  for (const auto &item : slice) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i}),
                    d.slice({Dim::X, 2, 4}).slice({dim, i})[item.name()]);
        EXPECT_ANY_THROW(item.slice({dim, item.dims()[dim]}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0}));
      }
    }
  }
}

TYPED_TEST(DataArrayView3DTest, slice_single_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto &item : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1}));
          for (scipp::index i = 0; i < item.dims()[dim]; ++i)
            EXPECT_EQ(item.slice({dim, i}), d.slice({dim, i})[item.name()]);
          EXPECT_ANY_THROW(item.slice({dim, item.dims()[dim]}));
        } else {
          EXPECT_ANY_THROW(item.slice({dim, 0}));
        }
      }
    }
  }
}

TYPED_TEST(DataArrayView3DTest, slice_length_0_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto &item : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1, -1}));
          for (scipp::index i = 0; i < item.dims()[dim]; ++i) {
            const auto slice = item.slice({dim, i, i + 0});
            EXPECT_EQ(slice, d.slice({dim, i, i + 0})[item.name()]);
            if (std::set(edgeDims).count(dim)) {
              EXPECT_EQ(slice.coords()[dim].dims()[dim], 1);
            }
          }
          EXPECT_NO_THROW(
              item.slice({dim, item.dims()[dim], item.dims()[dim] + 0}));
        } else {
          EXPECT_ANY_THROW(item.slice({dim, 0, 0}));
        }
      }
    }
  }
}

TYPED_TEST(DataArrayView3DTest, slice_length_1_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto &item : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1, 0}));
          for (scipp::index i = 0; i < item.dims()[dim]; ++i) {
            const auto slice = item.slice({dim, i, i + 1});
            EXPECT_EQ(slice, d.slice({dim, i, i + 1})[item.name()]);
            if (std::set(edgeDims).count(dim)) {
              EXPECT_EQ(slice.coords()[dim].dims()[dim], 2);
            }
          }
          EXPECT_ANY_THROW(
              item.slice({dim, item.dims()[dim], item.dims()[dim] + 1}));
        } else {
          EXPECT_ANY_THROW(item.slice({dim, 0, 0}));
        }
      }
    }
  }
}

TYPED_TEST(DataArrayView3DTest, slice_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto &item : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1, 1}));
          for (scipp::index i = 0; i < item.dims()[dim] - 1; ++i) {
            const auto slice = item.slice({dim, i, i + 2});
            EXPECT_EQ(slice, d.slice({dim, i, i + 2})[item.name()]);
            if (std::set(edgeDims).count(dim)) {
              EXPECT_EQ(slice.coords()[dim].dims()[dim], 3);
            }
          }
          EXPECT_ANY_THROW(
              item.slice({dim, item.dims()[dim], item.dims()[dim] + 2}));
        } else {
          EXPECT_ANY_THROW(item.slice({dim, 0, 2}));
        }
      }
    }
  }
}

class CoordToAttrMappingTest : public ::testing::Test {
protected:
  Variable x = makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  DataArray a{x, {{Dim::X, x}}};
};

template <class T> void test_coord_to_attr_mapping(T &o) {
  EXPECT_FALSE(o.attrs().contains("x"));
  EXPECT_FALSE(o.slice({Dim::X, 2, 3}).attrs().contains("x"));
  EXPECT_TRUE(o.slice({Dim::X, 2}).attrs().contains("x"));
  EXPECT_EQ(o.slice({Dim::X, 2}).attrs()["x"], 3.0 * units::one);
  EXPECT_TRUE(o.slice({Dim::X, 2, 3}).slice({Dim::X, 0}).attrs().contains("x"));
  EXPECT_EQ(o.slice({Dim::X, 2, 3}).slice({Dim::X, 0}).attrs()["x"],
            3.0 * units::one);
}

template <class T> void test_dataset_coord_to_attr_mapping(T &o) {
  EXPECT_FALSE(o.attrs().contains("x"));
  EXPECT_FALSE(o.slice({Dim::X, 2, 3}).attrs().contains("x"));
  // No mapping to attrs of *dataset*
  EXPECT_FALSE(o.slice({Dim::X, 2}).attrs().contains("x"));
  // Mapped "aligned" coord of dataset to attr (unaligned coord) of item
  EXPECT_TRUE(o.slice({Dim::X, 2})["a"].attrs().contains("x"));
  EXPECT_EQ(o.slice({Dim::X, 2})["a"].attrs()["x"], 3.0 * units::one);
  EXPECT_TRUE(
      o.slice({Dim::X, 2, 3}).slice({Dim::X, 0})["a"].attrs().contains("x"));
  EXPECT_EQ(o.slice({Dim::X, 2, 3}).slice({Dim::X, 0})["a"].attrs()["x"],
            3.0 * units::one);
}

TEST_F(CoordToAttrMappingTest, DataArrayView) { test_coord_to_attr_mapping(a); }

TEST_F(CoordToAttrMappingTest, DataArrayConstView) {
  const DataArray &const_a = a;
  test_coord_to_attr_mapping(const_a);
}

TEST_F(CoordToAttrMappingTest, DatasetView) {
  Dataset d({{"a", a}});
  test_dataset_coord_to_attr_mapping(d);
}

TEST_F(CoordToAttrMappingTest, DatasetConstView) {
  const Dataset d({{"a", a}});
  test_dataset_coord_to_attr_mapping(d);
}
