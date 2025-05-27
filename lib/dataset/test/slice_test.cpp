// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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

auto drop_unaligned_edges(const DataArray &da) {
  // See #3148
  //
  // This function removes all offending coordinates to simplify
  // comparisons in tests.
  auto out = da;
  for (const auto &[key, coord] : da.coords()) {
    if (!coord.is_aligned() &&
        std::any_of(coord.dims().begin(), coord.dims().end(),
                    [&da](const Dim dim) { return !da.dims().contains(dim); }))
      out.coords().erase(key);
  }
  return out;
}

class Dataset3DTest : public ::testing::Test {
protected:
  Dataset3DTest() : dataset(factory.make("data_xyz")) {}

  Dataset
  datasetWithEdges(const std::initializer_list<sc_units::Dim::Id> &edgeDims) {
    auto d = dataset;
    for (const auto dim : edgeDims) {
      auto dims = dataset.coords()[dim].dims();
      dims.resize(dim, dims[dim] + 1);
      d.setCoord(dim, makeRandom(dims));
    }
    return d;
  }

  DatasetFactory factory{{{Dim::X, 4}, {Dim::Y, 5}, {Dim::Z, 6}}};
  Dataset dataset;
};

TEST_F(Dataset3DTest, dimension_extent_check_replace_with_edge_coord) {
  auto edge_coord = dataset;
  ASSERT_NO_THROW(edge_coord.setCoord(Dim::X, makeRandom({Dim::X, 5})));
  ASSERT_NE(edge_coord["data_xyz"], dataset["data_xyz"]);
  // Cannot incrementally grow.
  ASSERT_ANY_THROW(edge_coord.setCoord(Dim::X, makeRandom({Dim::X, 6})));
  // Reverse: Replace edge coord with non-edge coord works.
  ASSERT_NO_THROW(edge_coord.setCoord(Dim::X, makeRandom({Dim::X, 4})));
}

TEST_F(Dataset3DTest, dimension_extent_check_prevents_shrink) {
  // If we reduce the X extent to 3 we would have data defined at the edges,
  // which is forbidden.
  ASSERT_THROW(dataset.setCoord(Dim::X, makeRandom({Dim::X, 3})),
               except::DimensionError);
  ASSERT_THROW(dataset.setData("non_edge_data", makeRandom({Dim::X, 3})),
               except::DimensionError);
}

TEST_F(Dataset3DTest, dimension_extent_check_prevents_setting_edge_data) {
  ASSERT_THROW(dataset.setData("edge_data", makeRandom({Dim::X, 5})),
               except::DimensionError);
  ASSERT_NO_THROW(dataset.setCoord(Dim::X, makeRandom({Dim::X, 5})));
  // prevented even if we have coord on the edges
  ASSERT_THROW(dataset.setData("edge_data", makeRandom({Dim::X, 5})),
               except::DimensionError);
}

TEST_F(Dataset3DTest, dimension_extent_check_non_coord_dimension) {
  // This is the Y coordinate but has extra extent in X. It is interpreted as a
  // coord for X.
  ASSERT_NO_THROW(
      dataset.setCoord(Dim::Y, makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
  ASSERT_EQ(dataset.coords().dim_of(Dim::Y), Dim::X);
}

TEST_F(Dataset3DTest,
       dimension_extent_check_non_dimension_coord_dimension_fail) {
  // Non-dimension coords are considered coords for their inner dim. They can
  // only be edges for this dim.
  ASSERT_NO_THROW(dataset.setCoord(Dim("edge_labels"),
                                   makeRandom({{Dim::X, 4}, {Dim::Y, 6}})));
  ASSERT_NO_THROW(dataset.setCoord(Dim("edges_for_non_inner_dim"),
                                   makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
  ASSERT_THROW(dataset.setCoord(Dim("bad_edges_for_both_dims"),
                                makeRandom({{Dim::X, 5}, {Dim::Y, 6}})),
               except::DimensionError);
  ASSERT_NO_THROW(
      dataset.setCoord(Dim("edge_labels_can_be_set_also_if_edge_dim_coord"),
                       makeRandom({{Dim::X, 4}, {Dim::Y, 6}})));
  // Back to non-edge labels also works
  ASSERT_NO_THROW(dataset.setCoord(Dim("edge_labels"),
                                   makeRandom({{Dim::X, 4}, {Dim::Y, 5}})));
}

class Dataset3DTest_slice_x : public Dataset3DTest,
                              public ::testing::WithParamInterface<int> {
protected:
  Dataset reference(const scipp::index pos) {
    return copy(
        Dataset({{"data_xyz", dataset["data_xyz"].slice({Dim::X, pos})}},
                {{Dim{"scalar"}, dataset.coords()[Dim{"scalar"}]},
                 {Dim::Y, dataset.coords()[Dim::Y]},
                 {Dim::Z, dataset.coords()[Dim::Z]},
                 {Dim{"xy"}, dataset.coords()[Dim{"xy"}].slice({Dim::X, pos})},
                 {Dim("labels_y"), dataset.coords()[Dim("labels_y")]},
                 {Dim("labels_z"), dataset.coords()[Dim("labels_z")]}}));
  }
};
class Dataset3DTest_slice_y : public Dataset3DTest,
                              public ::testing::WithParamInterface<int> {};
class Dataset3DTest_slice_z : public Dataset3DTest,
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
    Dataset d({{"data_xyz", dataset["data_xyz"].slice({Dim::Y, begin, end})}});
    d.setCoord(Dim{"scalar"}, dataset.coords()[Dim{"scalar"}]);
    d.setCoord(Dim::X, dataset.coords()[Dim::X]);
    d.setCoord(Dim::Y, dataset.coords()[Dim::Y].slice({Dim::Y, begin, end}));
    d.setCoord(Dim::Z, dataset.coords()[Dim::Z]);
    d.setCoord(Dim{"xy"},
               dataset.coords()[Dim{"xy"}].slice({Dim::Y, begin, end}));
    d.setCoord(Dim("labels_x"), dataset.coords()[Dim("labels_x")]);
    d.setCoord(Dim("labels_y"),
               dataset.coords()[Dim("labels_y")].slice({Dim::Y, begin, end}));
    d.setCoord(Dim("labels_z"), dataset.coords()[Dim("labels_z")]);
    return d;
  }
};
class Dataset3DTest_slice_range_z : public Dataset3DTest,
                                    public ::testing::WithParamInterface<
                                        std::pair<scipp::index, scipp::index>> {
protected:
  Dataset reference(const scipp::index begin, const scipp::index end) {
    Dataset d({{"data_xyz", dataset["data_xyz"].slice({Dim::Z, begin, end})}});
    d.setCoord(Dim{"scalar"}, dataset.coords()[Dim{"scalar"}]);
    d.setCoord(Dim::X, dataset.coords()[Dim::X]);
    d.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
    d.setCoord(Dim::Z, dataset.coords()[Dim::Z].slice({Dim::Z, begin, end}));
    d.setCoord(Dim("labels_x"), dataset.coords()[Dim("labels_x")]);
    d.setCoord(Dim("labels_y"), dataset.coords()[Dim("labels_y")]);
    d.setCoord(Dim("labels_z"),
               dataset.coords()[Dim("labels_z")].slice({Dim::Z, begin, end}));
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

TEST_P(Dataset3DTest_slice_x, slice) {
  const auto pos = GetParam();
  auto expected = reference(pos);
  auto sliced = dataset.slice({Dim::X, pos});
  EXPECT_EQ(sliced, expected);

  // Non-range slice converts aligned coord to unaligned coord
  for (const auto &coord_name : {"x", "labels_x"})
    EXPECT_FALSE(sliced["data_xyz"].coords()[Dim{coord_name}].is_aligned());
}

TEST_P(Dataset3DTest_slice_y, slice) {
  const auto pos = GetParam();
  Dataset reference({{"data_xyz", dataset["data_xyz"].slice({Dim::Y, pos})}});
  reference.setCoord(Dim{"scalar"}, dataset.coords()[Dim{"scalar"}]);
  reference.setCoord(Dim::X, dataset.coords()[Dim::X]);
  reference.setCoord(Dim::Z, dataset.coords()[Dim::Z]);
  reference.setCoord(Dim{"xy"},
                     dataset.coords()[Dim{"xy"}].slice({Dim::Y, pos}));
  reference.setCoord(Dim("labels_x"), dataset.coords()[Dim("labels_x")]);
  reference.setCoord(Dim("labels_z"), dataset.coords()[Dim("labels_z")]);
  EXPECT_EQ(dataset.slice({Dim::Y, pos}), reference);
}

TEST_P(Dataset3DTest_slice_z, slice) {
  const auto pos = GetParam();
  Dataset reference({{"data_xyz", dataset["data_xyz"].slice({Dim::Z, pos})}});
  reference.setCoord(Dim{"scalar"}, dataset.coords()[Dim{"scalar"}]);
  reference.setCoord(Dim::X, dataset.coords()[Dim::X]);
  reference.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  reference.setCoord(Dim("labels_x"), dataset.coords()[Dim("labels_x")]);
  reference.setCoord(Dim("labels_y"), dataset.coords()[Dim("labels_y")]);
  EXPECT_EQ(dataset.slice({Dim::Z, pos}), reference);
}

TEST_P(Dataset3DTest_slice_range_x, slice) {
  const auto [begin, end] = GetParam();
  Dataset reference(
      {{"data_xyz", dataset["data_xyz"].slice({Dim::X, begin, end})}});
  reference.setCoord(Dim{"scalar"}, dataset.coords()[Dim{"scalar"}]);
  reference.setCoord(Dim::X,
                     dataset.coords()[Dim::X].slice({Dim::X, begin, end}));
  reference.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  reference.setCoord(Dim::Z, dataset.coords()[Dim::Z]);
  reference.setCoord(Dim{"xy"},
                     dataset.coords()[Dim{"xy"}].slice({Dim::X, begin, end}));
  reference.setCoord(Dim("labels_x"), dataset.coords()[Dim("labels_x")].slice(
                                          {Dim::X, begin, end}));
  reference.setCoord(Dim("labels_y"), dataset.coords()[Dim("labels_y")]);
  reference.setCoord(Dim("labels_z"), dataset.coords()[Dim("labels_z")]);
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
  referenceWithEdges.setCoord(
      Dim::Y, yEdges.slice({Dim::Y, begin, begin == end ? end : end + 1}));
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
  referenceWithEdges.setCoord(
      Dim::Z, zEdges.slice({Dim::Z, begin, begin == end ? end : end + 1}));
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

using DataArrayViewTypes = ::testing::Types<DataArray, const DataArray>;

template <typename T> class DataArrayView3DTest : public Dataset3DTest {
protected:
  using dataset_type =
      std::conditional_t<std::is_same_v<T, DataArray>, Dataset, const Dataset>;

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
            EXPECT_EQ(drop_unaligned_edges(item.slice({dim, i})),
                      d.slice({dim, i})[item.name()]);
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
              // Note: NOT 1 edge
              EXPECT_EQ(slice.coords()[dim].dims()[dim], 0);
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

TEST(SliceWithStrideTest, throws_SliceError_if_bin_edges_along_slice_dim) {
  DataArray da(makeVariable<double>(Dims{Dim::X}, Shape{4}));
  da.coords().set(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{5}));
  Slice params(Dim::X, 0, 4, 2);
  EXPECT_THROW(da.slice(params), except::SliceError);
}

TEST(SliceWithStrideTest, works_if_bin_edges_along_unrelated_dim) {
  DataArray da(makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{4, 4}));
  da.coords().set(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{5}));
  Slice params(Dim::Y, 0, 4, 2);
  EXPECT_NO_THROW_DISCARD(da.slice(params));
}
