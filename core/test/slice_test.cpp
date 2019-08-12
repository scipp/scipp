// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include <numeric>

#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::core;

class Dataset3DTest : public ::testing::Test {
protected:
  Dataset3DTest() : dataset(factory.make()) {}

  Dataset datasetWithEdges(const std::initializer_list<Dim> &edgeDims) {
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
  // Minor implementation shortcoming: Currently we cannot go back to non-edges.
  ASSERT_ANY_THROW(edge_coord.setCoord(Dim::X, makeRandom({Dim::X, 4})));
}

TEST_F(Dataset3DTest,
       dimension_extent_check_prevents_non_edge_coord_with_edge_data) {
  // If we reduce the X extent to 3 we would have data defined at the edges, but
  // the coord is not. This is forbidden.
  ASSERT_ANY_THROW(dataset.setCoord(Dim::X, makeRandom({Dim::X, 3})));
  // We *can* set data with X extent 3. The X coord is now bin edges, and other
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

TEST_F(Dataset3DTest, data_check_upon_setting_sparse_coordinates) {
  Dataset sparse;
  auto data_var = makeVariable<double>({Dim::X, Dimensions::Sparse});
  data_var.sparseValues<double>()[0] = {1, 1, 1};
  auto coords_var = makeVariable<double>({Dim::X, Dimensions::Sparse});
  coords_var.sparseValues<double>()[0] = {1, 2, 3};
  sparse.setData("sparse_x", data_var);
  // The following should be OK. Data is sparse.
  sparse.setSparseCoord("sparse_x", coords_var);

  // Check with dense data
  ASSERT_THROW(
      dataset.setSparseCoord(
          "data_x", makeVariable<double>({Dim::X, Dimensions::Sparse})),
      std::runtime_error);
}

TEST_F(Dataset3DTest, dimension_extent_check_labels_dimension_fail) {
  // We cannot have labels on edges unless the coords are also edges. Note the
  // slight inconsistency though: Labels are typically though of as being for a
  // particular dimension (the inner one), but we can have labels on edges also
  // for the other dimensions (x in this case), just like data.
  ASSERT_ANY_THROW(
      dataset.setLabels("bad_labels", makeRandom({{Dim::X, 4}, {Dim::Y, 6}})));
  ASSERT_ANY_THROW(
      dataset.setLabels("bad_labels", makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
  dataset.setCoord(Dim::Y, makeRandom({{Dim::X, 4}, {Dim::Y, 6}}));
  ASSERT_ANY_THROW(
      dataset.setLabels("bad_labels", makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
  dataset.setCoord(Dim::X, makeRandom({Dim::X, 5}));
  ASSERT_NO_THROW(
      dataset.setLabels("good_labels", makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
  ASSERT_NO_THROW(
      dataset.setLabels("good_labels", makeRandom({{Dim::X, 5}, {Dim::Y, 6}})));
  ASSERT_NO_THROW(
      dataset.setLabels("good_labels", makeRandom({{Dim::X, 4}, {Dim::Y, 6}})));
  ASSERT_NO_THROW(
      dataset.setLabels("good_labels", makeRandom({{Dim::X, 4}, {Dim::Y, 5}})));
}

class Dataset3DTest_slice_x : public Dataset3DTest,
                              public ::testing::WithParamInterface<int> {
protected:
  Dataset reference(const scipp::index pos) {
    Dataset d;
    d.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
    d.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
    d.setCoord(Dim::Z, dataset.coords()[Dim::Z].slice({Dim::X, pos}));
    d.setLabels("labels_xy",
                dataset.labels()["labels_xy"].slice({Dim::X, pos}));
    d.setLabels("labels_z", dataset.labels()["labels_z"]);
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
class Dataset3DTest_slice_sparse : public Dataset3DTest,
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
    d.setLabels("labels_x", dataset.labels()["labels_x"]);
    d.setLabels("labels_xy",
                dataset.labels()["labels_xy"].slice({Dim::Y, begin, end}));
    d.setLabels("labels_z", dataset.labels()["labels_z"]);
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
    d.setLabels("labels_x", dataset.labels()["labels_x"]);
    d.setLabels("labels_xy", dataset.labels()["labels_xy"]);
    d.setLabels("labels_z",
                dataset.labels()["labels_z"].slice({Dim::Z, begin, end}));
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
  std::array<std::pair<index, index>, (size * size + size) / 2> pairs;
  index i = 0;
  for (index first = 0; first <= max; ++first)
    for (index second = first + 0; second <= max; ++second) {
      pairs[i].first = first;
      pairs[i].second = second;
      ++i;
    }
  return pairs;
}

constexpr auto ranges_x = valid_ranges<4>();
constexpr auto ranges_y = valid_ranges<5>();
constexpr auto ranges_z = valid_ranges<6>();

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
INSTANTIATE_TEST_SUITE_P(AllPositions, Dataset3DTest_slice_sparse,
                         ::testing::Range(0, 2));

TEST_P(Dataset3DTest_slice_x, slice) {
  const auto pos = GetParam();
  EXPECT_EQ(dataset.slice({Dim::X, pos}), reference(pos));
}

TEST_P(Dataset3DTest_slice_sparse, slice) {
  Dataset ds;
  const auto pos = GetParam();
  auto var = makeVariable<double>(
      {{Dim::X, Dim::Y, Dim::Z}, {2, 2, Dimensions::Sparse}});
  var.sparseValues<double>()[0] = {1, 2, 3};
  var.sparseValues<double>()[1] = {4, 5, 6};
  var.sparseValues<double>()[2] = {7};
  var.sparseValues<double>()[3] = {8, 9};

  ds.setData("xyz_data", var);
  ds.setCoord(Dim::X, makeVariable<double>({Dim::X, 2}, {0, 1}));
  ds.setCoord(Dim::Y, makeVariable<double>({Dim::Y, 2}, {0, 1}));

  auto sliced = ds.slice({Dim::X, pos});
  auto data = sliced["xyz_data"].data().sparseValues<double>();
  EXPECT_EQ(data.size(), 2);
  scipp::core::sparse_container<double> expected =
      var.sparseValues<double>()[pos * 2];
  EXPECT_EQ(data[0], expected);
  expected = var.sparseValues<double>()[pos * 2 + 1];
  EXPECT_EQ(data[1], expected);
}

TEST_P(Dataset3DTest_slice_x, slice_bin_edges) {
  const auto pos = GetParam();
  auto datasetWithEdges = dataset;
  datasetWithEdges.setCoord(Dim::X, makeRandom({Dim::X, 5}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::X, pos}), reference(pos));
  EXPECT_EQ(datasetWithEdges.slice({Dim::X, pos}),
            dataset.slice({Dim::X, pos}));
}

TEST_P(Dataset3DTest_slice_y, slice) {
  const auto pos = GetParam();
  Dataset reference;
  reference.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
  reference.setCoord(Dim::X, dataset.coords()[Dim::X]);
  reference.setCoord(Dim::Z, dataset.coords()[Dim::Z].slice({Dim::Y, pos}));
  reference.setLabels("labels_x", dataset.labels()["labels_x"]);
  reference.setLabels("labels_z", dataset.labels()["labels_z"]);
  reference.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
  reference.setAttr("attr_x", dataset.attrs()["attr_x"]);
  reference.setData("data_xy", dataset["data_xy"].data().slice({Dim::Y, pos}));
  reference.setData("data_zyx",
                    dataset["data_zyx"].data().slice({Dim::Y, pos}));
  reference.setData("data_xyz",
                    dataset["data_xyz"].data().slice({Dim::Y, pos}));

  EXPECT_EQ(dataset.slice({Dim::Y, pos}), reference);
}

TEST_P(Dataset3DTest_slice_z, slice) {
  const auto pos = GetParam();
  Dataset reference;
  reference.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
  reference.setCoord(Dim::X, dataset.coords()[Dim::X]);
  reference.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  reference.setLabels("labels_x", dataset.labels()["labels_x"]);
  reference.setLabels("labels_xy", dataset.labels()["labels_xy"]);
  reference.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
  reference.setAttr("attr_x", dataset.attrs()["attr_x"]);
  reference.setData("data_zyx",
                    dataset["data_zyx"].data().slice({Dim::Z, pos}));
  reference.setData("data_xyz",
                    dataset["data_xyz"].data().slice({Dim::Z, pos}));

  EXPECT_EQ(dataset.slice({Dim::Z, pos}), reference);
}

TEST_P(Dataset3DTest_slice_range_x, slice) {
  const auto[begin, end] = GetParam();
  Dataset reference;
  reference.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
  reference.setCoord(Dim::X,
                     dataset.coords()[Dim::X].slice({Dim::X, begin, end}));
  reference.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  reference.setCoord(Dim::Z,
                     dataset.coords()[Dim::Z].slice({Dim::X, begin, end}));
  reference.setLabels("labels_x",
                      dataset.labels()["labels_x"].slice({Dim::X, begin, end}));
  reference.setLabels(
      "labels_xy", dataset.labels()["labels_xy"].slice({Dim::X, begin, end}));
  reference.setLabels("labels_z", dataset.labels()["labels_z"]);
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
  const auto[begin, end] = GetParam();
  EXPECT_EQ(dataset.slice({Dim::Y, begin, end}), reference(begin, end));
}

TEST_P(Dataset3DTest_slice_range_y, slice_with_edges) {
  const auto[begin, end] = GetParam();
  auto datasetWithEdges = dataset;
  const auto yEdges = makeRandom({Dim::Y, 6});
  datasetWithEdges.setCoord(Dim::Y, yEdges);
  auto referenceWithEdges = reference(begin, end);
  // Is this the correct behavior for edges also in case the range is empty?
  referenceWithEdges.setCoord(Dim::Y, yEdges.slice({Dim::Y, begin, end + 1}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::Y, begin, end}), referenceWithEdges);
}

TEST_P(Dataset3DTest_slice_range_y, slice_with_z_edges) {
  const auto[begin, end] = GetParam();
  auto datasetWithEdges = dataset;
  const auto zEdges = makeRandom({{Dim::X, 4}, {Dim::Y, 5}, {Dim::Z, 7}});
  datasetWithEdges.setCoord(Dim::Z, zEdges);
  auto referenceWithEdges = reference(begin, end);
  referenceWithEdges.setCoord(Dim::Z, zEdges.slice({Dim::Y, begin, end}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::Y, begin, end}), referenceWithEdges);
}

TEST_P(Dataset3DTest_slice_range_z, slice) {
  const auto[begin, end] = GetParam();
  EXPECT_EQ(dataset.slice({Dim::Z, begin, end}), reference(begin, end));
}

TEST_P(Dataset3DTest_slice_range_z, slice_with_edges) {
  const auto[begin, end] = GetParam();
  auto datasetWithEdges = dataset;
  const auto zEdges = makeRandom({{Dim::X, 4}, {Dim::Y, 5}, {Dim::Z, 7}});
  datasetWithEdges.setCoord(Dim::Z, zEdges);
  auto referenceWithEdges = reference(begin, end);
  referenceWithEdges.setCoord(Dim::Z, zEdges.slice({Dim::Z, begin, end + 1}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::Z, begin, end}), referenceWithEdges);
}

TEST_F(Dataset3DTest, nested_slice) {
  for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
    EXPECT_EQ(dataset.slice({dim, 1, 3}, {dim, 1}), dataset.slice({dim, 2}));
  }
}

TEST_F(Dataset3DTest, nested_slice_range) {
  for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
    EXPECT_EQ(dataset.slice({dim, 1, 3}, {dim, 0, 2}),
              dataset.slice({dim, 1, 3}));
    EXPECT_EQ(dataset.slice({dim, 1, 3}, {dim, 1, 2}),
              dataset.slice({dim, 2, 3}));
  }
}

TEST_F(Dataset3DTest, nested_slice_range_bin_edges) {
  auto datasetWithEdges = dataset;
  datasetWithEdges.setCoord(Dim::X, makeRandom({Dim::X, 5}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::X, 1, 3}, {Dim::X, 0, 2}),
            datasetWithEdges.slice({Dim::X, 1, 3}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::X, 1, 3}, {Dim::X, 1, 2}),
            datasetWithEdges.slice({Dim::X, 2, 3}));
}

TEST_F(Dataset3DTest, commutative_slice) {
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2}),
            dataset.slice({Dim::Y, 2}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Y, 2}, {Dim::Z, 3, 4}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Z, 3, 4}, {Dim::Y, 2}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Z, 3, 4}, {Dim::X, 1, 3}, {Dim::Y, 2}));
}

TEST_F(Dataset3DTest, commutative_slice_range) {
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2, 4}),
            dataset.slice({Dim::Y, 2, 4}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2, 4}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Y, 2, 4}, {Dim::Z, 3, 4}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2, 4}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Z, 3, 4}, {Dim::Y, 2, 4}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2, 4}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Z, 3, 4}, {Dim::X, 1, 3}, {Dim::Y, 2, 4}));
}

using DataProxyTypes = ::testing::Types<DataProxy, DataConstProxy>;

template <typename T> class DataProxy3DTest : public Dataset3DTest {
protected:
  using dataset_type =
      std::conditional_t<std::is_same_v<T, DataProxy>, Dataset, const Dataset>;

  dataset_type &dataset() { return Dataset3DTest::dataset; }
};

TYPED_TEST_SUITE(DataProxy3DTest, DataProxyTypes);

// We have tests that ensure that Dataset::slice is correct (and its item access
// returns the correct data), so we rely on that for verifying the results of
// slicing DataProxy.
TYPED_TEST(DataProxy3DTest, slice_single) {
  auto &d = TestFixture::dataset();
  for (const auto[name, item] : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i}), d.slice({dim, i})[name]);
        EXPECT_ANY_THROW(item.slice({dim, item.dims()[dim]}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0}));
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_length_0) {
  auto &d = TestFixture::dataset();
  for (const auto[name, item] : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1, -1}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i, i + 0}),
                    d.slice({dim, i, i + 0})[name]);
        EXPECT_ANY_THROW(
            item.slice({dim, item.dims()[dim], item.dims()[dim] + 0}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0, 0}));
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_length_1) {
  auto &d = TestFixture::dataset();
  for (const auto[name, item] : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1, 0}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i, i + 1}),
                    d.slice({dim, i, i + 1})[name]);
        EXPECT_ANY_THROW(
            item.slice({dim, item.dims()[dim], item.dims()[dim] + 1}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0, 0}));
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice) {
  auto &d = TestFixture::dataset();
  for (const auto[name, item] : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1, 1}));
        for (scipp::index i = 0; i < item.dims()[dim] - 1; ++i)
          EXPECT_EQ(item.slice({dim, i, i + 2}),
                    d.slice({dim, i, i + 2})[name]);
        EXPECT_ANY_THROW(
            item.slice({dim, item.dims()[dim], item.dims()[dim] + 2}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0, 2}));
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_slice_range) {
  auto &d = TestFixture::dataset();
  const auto slice = d.slice({Dim::X, 2, 4});
  // Slice proxy created from DatasetProxy as opposed to directly from Dataset.
  for (const auto[name, item] : slice) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i}),
                    d.slice({Dim::X, 2, 4}, {dim, i})[name]);
        EXPECT_ANY_THROW(item.slice({dim, item.dims()[dim]}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0}));
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_single_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto[name, item] : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1}));
          for (scipp::index i = 0; i < item.dims()[dim]; ++i)
            EXPECT_EQ(item.slice({dim, i}), d.slice({dim, i})[name]);
          EXPECT_ANY_THROW(item.slice({dim, item.dims()[dim]}));
        } else {
          EXPECT_ANY_THROW(item.slice({dim, 0}));
        }
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_length_0_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto[name, item] : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1, -1}));
          for (scipp::index i = 0; i < item.dims()[dim]; ++i) {
            const auto slice = item.slice({dim, i, i + 0});
            EXPECT_EQ(slice, d.slice({dim, i, i + 0})[name]);
            if (std::set(edgeDims).count(dim)) {
              EXPECT_EQ(slice.coords()[dim].dims()[dim], 1);
            }
          }
          EXPECT_ANY_THROW(
              item.slice({dim, item.dims()[dim], item.dims()[dim] + 0}));
        } else {
          EXPECT_ANY_THROW(item.slice({dim, 0, 0}));
        }
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_length_1_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto[name, item] : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1, 0}));
          for (scipp::index i = 0; i < item.dims()[dim]; ++i) {
            const auto slice = item.slice({dim, i, i + 1});
            EXPECT_EQ(slice, d.slice({dim, i, i + 1})[name]);
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

TYPED_TEST(DataProxy3DTest, slice_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto[name, item] : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1, 1}));
          for (scipp::index i = 0; i < item.dims()[dim] - 1; ++i) {
            const auto slice = item.slice({dim, i, i + 2});
            EXPECT_EQ(slice, d.slice({dim, i, i + 2})[name]);
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
