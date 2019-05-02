// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "dataset_next.h"
#include "dimensions.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::core::next;

TEST(DatasetNext, construct_default) { ASSERT_NO_THROW(next::Dataset d); }

TEST(DatasetNext, empty) {
  next::Dataset d;
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(d.size(), 0);
}

TEST(DatasetNext, coords) {
  next::Dataset d;
  ASSERT_NO_THROW(d.coords());
}

TEST(DatasetNext, labels) {
  next::Dataset d;
  ASSERT_NO_THROW(d.labels());
}

TEST(DatasetNext, bad_item_access) {
  next::Dataset d;
  ASSERT_ANY_THROW(d[""]);
  ASSERT_ANY_THROW(d["abc"]);
}

TEST(DatasetNext, setCoord) {
  next::Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

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

TEST(DatasetNext, setLabels) {
  next::Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 0);

  ASSERT_NO_THROW(d.setLabels("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 1);

  ASSERT_NO_THROW(d.setLabels("b", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 2);

  ASSERT_NO_THROW(d.setLabels("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 2);
}

TEST(DatasetNext, setValues_setVariances) {
  next::Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_NO_THROW(d.setValues("a", var));
  ASSERT_EQ(d.size(), 1);

  ASSERT_NO_THROW(d.setValues("b", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_NO_THROW(d.setValues("a", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_NO_THROW(d.setVariances("a", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_ANY_THROW(d.setVariances("c", var));
}
TEST(DatasetNext, setLabels_with_name_matching_data_name) {
  next::Dataset d;
  d.setValues("a", makeVariable<double>({Dim::X, 3}));
  d.setValues("b", makeVariable<double>({Dim::X, 3}));

  // It is possible to set labels with a name matching data. However, there is
  // no special meaning attached to this. In particular it is *not* linking the
  // labels to that data item.
  ASSERT_NO_THROW(d.setLabels("a", makeVariable<double>({})));
  ASSERT_EQ(d.size(), 2);
  ASSERT_EQ(d.labels().size(), 1);
  ASSERT_EQ(d["a"].labels().size(), 1);
  ASSERT_EQ(d["b"].labels().size(), 1);
}

TEST(DatasetNext, setVariances_dtype_mismatch) {
  next::Dataset d;
  d.setValues("", makeVariable<double>({}));
  ASSERT_ANY_THROW(d.setVariances("", makeVariable<float>({})));
  ASSERT_NO_THROW(d.setVariances("", makeVariable<double>({})));
}

TEST(DatasetNext, setVariances_unit_mismatch) {
  next::Dataset d;
  auto values = makeVariable<double>({});
  values.setUnit(units::m);
  d.setValues("", values);
  auto variances = makeVariable<double>({});
  ASSERT_ANY_THROW(d.setVariances("", variances));
  variances.setUnit(units::m);
  ASSERT_ANY_THROW(d.setVariances("", variances));
  variances.setUnit(units::m * units::m);
  ASSERT_NO_THROW(d.setVariances("", variances));
}

TEST(DatasetNext, setVariances_dimensions_mismatch) {
  next::Dataset d;
  d.setValues("", makeVariable<double>({}));
  ASSERT_ANY_THROW(d.setVariances("", makeVariable<double>({Dim::X, 1})));
  ASSERT_NO_THROW(d.setVariances("", makeVariable<double>({})));
}

TEST(DatasetNext, setVariances_sparseDim_mismatch) {
  next::Dataset d;
  d.setValues("", makeSparseVariable<double>({}, Dim::X));
  ASSERT_ANY_THROW(d.setVariances("", makeVariable<double>({Dim::X, 1})));
  ASSERT_ANY_THROW(d.setVariances("", makeVariable<double>({})));
  ASSERT_ANY_THROW(d.setVariances("", makeSparseVariable<double>({}, Dim::Y)));
  ASSERT_ANY_THROW(
      d.setVariances("", makeSparseVariable<double>({Dim::X, 1}, Dim::X)));
  ASSERT_NO_THROW(d.setVariances("", makeSparseVariable<double>({}, Dim::X)));
}

TEST(DatasetNext, setValues_dtype_mismatch) {
  next::Dataset d;
  d.setValues("", makeVariable<double>({}));
  d.setVariances("", makeVariable<double>({}));
  ASSERT_ANY_THROW(d.setValues("", makeVariable<float>({})));
  ASSERT_NO_THROW(d.setValues("", makeVariable<double>({})));
}

TEST(DatasetNext, setValues_dimensions_mismatch) {
  next::Dataset d;
  d.setValues("", makeVariable<double>({}));
  d.setVariances("", makeVariable<double>({}));
  ASSERT_ANY_THROW(d.setValues("", makeVariable<double>({Dim::X, 1})));
  ASSERT_NO_THROW(d.setValues("", makeVariable<double>({})));
}

TEST(DatasetNext, setValues_sparseDim_mismatch) {
  next::Dataset d;
  d.setValues("", makeSparseVariable<double>({}, Dim::X));
  d.setVariances("", makeSparseVariable<double>({}, Dim::X));
  ASSERT_ANY_THROW(d.setValues("", makeVariable<double>({Dim::X, 1})));
  ASSERT_ANY_THROW(d.setValues("", makeVariable<double>({})));
  ASSERT_ANY_THROW(d.setValues("", makeSparseVariable<double>({}, Dim::Y)));
  ASSERT_ANY_THROW(
      d.setValues("", makeSparseVariable<double>({Dim::X, 1}, Dim::X)));
  ASSERT_NO_THROW(d.setValues("", makeSparseVariable<double>({}, Dim::X)));
}

TEST(DatasetNext, setSparseCoord_not_sparse_fail) {
  next::Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_ANY_THROW(d.setSparseCoord("a", var));
}

TEST(DatasetNext, setSparseCoord) {
  next::Dataset d;
  const auto var = makeSparseVariable<double>({Dim::X, 3}, Dim::Y);

  ASSERT_NO_THROW(d.setSparseCoord("a", var));
  ASSERT_EQ(d.size(), 1);
  ASSERT_NO_THROW(d["a"]);
}

TEST(DatasetNext, setSparseLabels_missing_values_or_coord) {
  next::Dataset d;
  const auto sparse = makeSparseVariable<double>({}, Dim::X);

  ASSERT_ANY_THROW(d.setSparseLabels("a", "x", sparse));
  d.setSparseCoord("a", sparse);
  ASSERT_NO_THROW(d.setSparseLabels("a", "x", sparse));
}

TEST(DatasetNext, setSparseLabels_not_sparse_fail) {
  next::Dataset d;
  const auto dense = makeVariable<double>({});
  const auto sparse = makeSparseVariable<double>({}, Dim::X);

  d.setSparseCoord("a", sparse);
  ASSERT_ANY_THROW(d.setSparseLabels("a", "x", dense));
}

TEST(DatasetNext, setSparseLabels) {
  next::Dataset d;
  const auto sparse = makeSparseVariable<double>({}, Dim::X);
  d.setSparseCoord("a", sparse);

  ASSERT_NO_THROW(d.setSparseLabels("a", "x", sparse));
  ASSERT_EQ(d.size(), 1);
  ASSERT_NO_THROW(d["a"]);
  ASSERT_EQ(d["a"].labels().size(), 1);
}

TEST(DatasetNext, iterators_empty_dataset) {
  next::Dataset d;
  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());
  EXPECT_EQ(d.begin(), d.end());
}

TEST(DatasetNext, iterators_only_coords) {
  next::Dataset d;
  d.setCoord(Dim::X, makeVariable<double>({}));
  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());
  EXPECT_EQ(d.begin(), d.end());
}

TEST(DatasetNext, iterators_only_labels) {
  next::Dataset d;
  d.setLabels("a", makeVariable<double>({}));
  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());
  EXPECT_EQ(d.begin(), d.end());
}

TEST(DatasetNext, iterators) {
  next::Dataset d;
  d.setValues("a", makeVariable<double>({}));
  d.setValues("b", makeVariable<float>({}));
  d.setValues("c", makeVariable<int64_t>({}));

  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());

  auto it = d.begin();
  ASSERT_NE(it, d.end());
  EXPECT_EQ(it->first, "a");

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, d.end());
  EXPECT_EQ(it->first, "b");

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, d.end());
  EXPECT_EQ(it->first, "c");

  ASSERT_NO_THROW(++it);
  ASSERT_EQ(it, d.end());
}

TEST(DatasetNext, iterators_return_types) {
  next::Dataset d;
  ASSERT_TRUE((std::is_same_v<decltype(d.begin()->second), DataProxy>));
  ASSERT_TRUE((std::is_same_v<decltype(d.end()->second), DataProxy>));
}

TEST(DatasetNext, const_iterators_return_types) {
  const next::Dataset d;
  ASSERT_TRUE((std::is_same_v<decltype(d.begin()->second), DataConstProxy>));
  ASSERT_TRUE((std::is_same_v<decltype(d.end()->second), DataConstProxy>));
}

TEST(DatasetNext, slice) {
  next::Dataset d;
  d.setCoord(Dim::X, makeVariable<double>({Dim::X, 3}, {1, 2, 3}));
  d.setCoord(Dim::Y, makeVariable<double>({Dim::Y, 4}, {4, 5, 6, 7}));
  const auto data_x = makeVariable<double>({Dim::X, 3}, {3, 2, 1});
  const auto data_y = makeVariable<double>({Dim::Y, 4}, {7, 6, 5, 4});
  const auto data_xy = makeVariable<double>(
      {{Dim::Y, 4}, {Dim::X, 3}}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  d.setValues("x", data_x);
  d.setValues("y", data_y);
  d.setValues("xy", data_xy);

  const auto slice = d.slice({Dim::Y, 1, 3});
  EXPECT_EQ(slice.size(), 2);
  EXPECT_ANY_THROW(slice["x"]);
  EXPECT_NO_THROW(slice["y"]);
  EXPECT_NO_THROW(slice["xy"]);

  std::map<std::string, ConstVariableSlice> expected;
  expected.emplace("y", data_y.slice({Dim::Y, 1, 3}));
  expected.emplace("xy", data_xy.slice({Dim::Y, 1, 3}));

  std::map<std::string, ConstVariableSlice> actual;
  for (const auto & [ name, data ] : slice)
    EXPECT_TRUE(actual.emplace(name, data.values()).second);
  EXPECT_EQ(actual, expected);
}

TEST(CoordsConstProxy, empty) {
  next::Dataset d;
  const auto coords = d.coords();
  ASSERT_TRUE(coords.empty());
  ASSERT_EQ(coords.size(), 0);
}

TEST(CoordsConstProxy, bad_item_access) {
  next::Dataset d;
  const auto coords = d.coords();
  ASSERT_ANY_THROW(coords[Dim::X]);
}

TEST(CoordsConstProxy, item_access) {
  next::Dataset d;
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 2}, {4, 5});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);

  const auto coords = d.coords();
  ASSERT_EQ(coords[Dim::X], x);
  ASSERT_EQ(coords[Dim::Y], y);
}

TEST(CoordsConstProxy, item_write) {
  next::Dataset d;
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 2}, {4, 5});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);

  const auto coords = d.coords();
  coords[Dim::X].values<double>()[0] += 0.5;
  coords[Dim::Y].values<double>()[0] += 0.5;
  ASSERT_TRUE(equals(coords[Dim::X].values<double>(), {1.5, 2.0, 3.0}));
  ASSERT_TRUE(equals(coords[Dim::Y].values<double>(), {4.5, 5.0}));
}

TEST(CoordsConstProxy, iterators_empty_coords) {
  next::Dataset d;
  const auto coords = d.coords();

  ASSERT_NO_THROW(coords.begin());
  ASSERT_NO_THROW(coords.end());
  EXPECT_EQ(coords.begin(), coords.end());
}

TEST(CoordsConstProxy, iterators) {
  next::Dataset d;
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 2}, {4, 5});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  const auto coords = d.coords();

  ASSERT_NO_THROW(coords.begin());
  ASSERT_NO_THROW(coords.end());

  auto it = coords.begin();
  ASSERT_NE(it, coords.end());
  EXPECT_EQ(it->first, Dim::X);

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, coords.end());
  EXPECT_EQ(it->first, Dim::Y);

  ASSERT_NO_THROW(++it);
  ASSERT_EQ(it, coords.end());
}

TEST(CoordsConstProxy, slice) {
  next::Dataset d;
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 2}, {1, 2});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  const auto coords = d.coords();

  const auto sliceX = coords.slice({Dim::X, 1});
  EXPECT_ANY_THROW(sliceX[Dim::X]);
  EXPECT_EQ(sliceX[Dim::Y], y);

  const auto sliceDX = coords.slice({Dim::X, 1, 2});
  EXPECT_EQ(sliceDX[Dim::X], x.slice({Dim::X, 1, 2}));
  EXPECT_EQ(sliceDX[Dim::Y], y);

  const auto sliceY = coords.slice({Dim::Y, 1});
  EXPECT_EQ(sliceY[Dim::X], x);
  EXPECT_ANY_THROW(sliceY[Dim::Y]);

  const auto sliceDY = coords.slice({Dim::Y, 1, 2});
  EXPECT_EQ(sliceDY[Dim::X], x);
  EXPECT_EQ(sliceDY[Dim::Y], y.slice({Dim::Y, 1, 2}));
}

auto make_dataset_2d_coord_x_1d_coord_y() {
  next::Dataset d;
  const auto x =
      makeVariable<double>({{Dim::X, 3}, {Dim::Y, 2}}, {1, 2, 3, 4, 5, 6});
  const auto y = makeVariable<double>({Dim::Y, 2}, {1, 2});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  return d;
}

TEST(CoordsConstProxy, slice_2D_coord) {
  const auto d = make_dataset_2d_coord_x_1d_coord_y();
  const auto coords = d.coords();

  const auto sliceX = coords.slice({Dim::X, 1});
  EXPECT_ANY_THROW(sliceX[Dim::X]);
  EXPECT_EQ(sliceX[Dim::Y], coords[Dim::Y]);

  const auto sliceDX = coords.slice({Dim::X, 1, 2});
  EXPECT_EQ(sliceDX[Dim::X], coords[Dim::X].slice({Dim::X, 1, 2}));
  EXPECT_EQ(sliceDX[Dim::Y], coords[Dim::Y]);

  const auto sliceY = coords.slice({Dim::Y, 1});
  EXPECT_EQ(sliceY[Dim::X], coords[Dim::X].slice({Dim::Y, 1}));
  EXPECT_ANY_THROW(sliceY[Dim::Y]);

  const auto sliceDY = coords.slice({Dim::Y, 1, 2});
  EXPECT_EQ(sliceDY[Dim::X], coords[Dim::X].slice({Dim::Y, 1, 2}));
  EXPECT_EQ(sliceDY[Dim::Y], coords[Dim::Y].slice({Dim::Y, 1, 2}));
}

auto check_slice_of_slice = [](const auto &dataset, const auto slice) {
  EXPECT_EQ(slice[Dim::X],
            dataset.coords()[Dim::X].slice({Dim::X, 1, 3}).slice({Dim::Y, 1}));
  EXPECT_ANY_THROW(slice[Dim::Y]);
};

TEST(CoordsConstProxy, slice_of_slice) {
  const auto d = make_dataset_2d_coord_x_1d_coord_y();
  const auto cs = d.coords();

  check_slice_of_slice(d, cs.slice({Dim::X, 1, 3}).slice({Dim::Y, 1}));
  check_slice_of_slice(d, cs.slice({Dim::Y, 1}).slice({Dim::X, 1, 3}));
  check_slice_of_slice(d, cs.slice({Dim::X, 1, 3}, {Dim::Y, 1}));
  check_slice_of_slice(d, cs.slice({Dim::Y, 1}, {Dim::X, 1, 3}));
}

auto check_slice_of_slice_range = [](const auto &dataset, const auto slice) {
  EXPECT_EQ(
      slice[Dim::X],
      dataset.coords()[Dim::X].slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 2}));
  EXPECT_EQ(slice[Dim::Y], dataset.coords()[Dim::Y].slice({Dim::Y, 1, 2}));
};

TEST(CoordsConstProxy, slice_of_slice_range) {
  const auto d = make_dataset_2d_coord_x_1d_coord_y();
  const auto cs = d.coords();

  check_slice_of_slice_range(d, cs.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 2}));
  check_slice_of_slice_range(d, cs.slice({Dim::Y, 1, 2}).slice({Dim::X, 1, 3}));
  check_slice_of_slice_range(d, cs.slice({Dim::X, 1, 3}, {Dim::Y, 1, 2}));
  check_slice_of_slice_range(d, cs.slice({Dim::Y, 1, 2}, {Dim::X, 1, 3}));
}

TEST(CoordsConstProxy, slice_return_type) {
  const next::Dataset d;
  ASSERT_TRUE((std::is_same_v<decltype(d.coords().slice({Dim::X, 0})),
                              CoordsConstProxy>));
}

TEST(CoordsProxy, slice_return_type) {
  next::Dataset d;
  ASSERT_TRUE(
      (std::is_same_v<decltype(d.coords().slice({Dim::X, 0})), CoordsProxy>));
}

TEST(CoordsProxy, modify_slice) {
  auto d = make_dataset_2d_coord_x_1d_coord_y();
  const auto coords = d.coords();

  const auto slice = coords.slice({Dim::X, 1, 2});
  for (auto &x : slice[Dim::X].values<double>())
    x = 0.0;

  const auto reference =
      makeVariable<double>({{Dim::X, 3}, {Dim::Y, 2}}, {1, 2, 0, 0, 5, 6});
  EXPECT_EQ(d.coords()[Dim::X], reference);
}

TEST(CoordsConstProxy, slice_bin_edges_with_2D_coord) {
  next::Dataset d;
  const auto x = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  const auto y_edges = makeVariable<double>({Dim::Y, 3}, {1, 2, 3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y_edges);
  const auto coords = d.coords();

  const auto sliceX = coords.slice({Dim::X, 1});
  EXPECT_ANY_THROW(sliceX[Dim::X]);
  EXPECT_EQ(sliceX[Dim::Y], coords[Dim::Y]);

  const auto sliceDX = coords.slice({Dim::X, 1, 2});
  EXPECT_EQ(sliceDX[Dim::X].dims(), Dimensions({{Dim::Y, 2}, {Dim::X, 1}}));
  EXPECT_EQ(sliceDX[Dim::Y], coords[Dim::Y]);

  const auto sliceY = coords.slice({Dim::Y, 1});
  // TODO Would it be more consistent to preserve X with 0 thickness?
  EXPECT_ANY_THROW(sliceY[Dim::X]);
  EXPECT_ANY_THROW(sliceY[Dim::Y]);

  const auto sliceY_edge = coords.slice({Dim::Y, 1, 2});
  EXPECT_EQ(sliceY_edge[Dim::X], coords[Dim::X].slice({Dim::Y, 1, 1}));
  EXPECT_EQ(sliceY_edge[Dim::Y], coords[Dim::Y].slice({Dim::Y, 1, 2}));

  const auto sliceY_bin = coords.slice({Dim::Y, 1, 3});
  EXPECT_EQ(sliceY_bin[Dim::X], coords[Dim::X].slice({Dim::Y, 1, 2}));
  EXPECT_EQ(sliceY_bin[Dim::Y], coords[Dim::Y].slice({Dim::Y, 1, 3}));
}

// Using typed tests for common functionality of DataProxy and DataConstProxy.
template <typename T> class DataProxyTest : public ::testing::Test {
public:
  using proxy_type = T;
};

using DataProxyTypes = ::testing::Types<next::Dataset, const next::Dataset>;
TYPED_TEST_CASE(DataProxyTest, DataProxyTypes);

TYPED_TEST(DataProxyTest, isSparse_sparseDim) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);

  d.setValues("dense", makeVariable<double>({}));
  ASSERT_FALSE(d_ref["dense"].isSparse());
  ASSERT_EQ(d_ref["dense"].sparseDim(), Dim::Invalid);

  d.setValues("sparse_data", makeSparseVariable<double>({}, Dim::X));
  ASSERT_TRUE(d_ref["sparse_data"].isSparse());
  ASSERT_EQ(d_ref["sparse_data"].sparseDim(), Dim::X);

  d.setSparseCoord("sparse_coord", makeSparseVariable<double>({}, Dim::X));
  ASSERT_TRUE(d_ref["sparse_coord"].isSparse());
  ASSERT_EQ(d_ref["sparse_coord"].sparseDim(), Dim::X);
}

TYPED_TEST(DataProxyTest, dims) {
  next::Dataset d;
  const auto dense = makeVariable<double>({{Dim::X, 1}, {Dim::Y, 2}});
  const auto sparse =
      makeSparseVariable<double>({{Dim::X, 1}, {Dim::Y, 2}}, Dim::Z);
  typename TestFixture::proxy_type &d_ref(d);

  d.setValues("dense", dense);
  ASSERT_EQ(d_ref["dense"].dims(), dense.dims());

  // Sparse dimension is currently not included in dims(). It is unclear whether
  // this is the right choice. An unfinished idea involves returning
  // std::tuple<std::span<const Dim>, std::optional<Dim>> instead, using `auto [
  // dims, sparse ] = data.dims();`.
  d.setValues("sparse_data", sparse);
  ASSERT_EQ(d_ref["sparse_data"].dims(), dense.dims());
  ASSERT_EQ(d_ref["sparse_data"].dims(), sparse.dims());

  d.setSparseCoord("sparse_coord", sparse);
  ASSERT_EQ(d_ref["sparse_coord"].dims(), dense.dims());
  ASSERT_EQ(d_ref["sparse_coord"].dims(), sparse.dims());
}

TYPED_TEST(DataProxyTest, dims_with_extra_coords) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
  const auto var = makeVariable<double>({Dim::X, 3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setValues("a", var);

  ASSERT_EQ(d_ref["a"].dims(), var.dims());
}

TYPED_TEST(DataProxyTest, unit) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);

  d.setValues("dense", makeVariable<double>({}));
  EXPECT_EQ(d_ref["dense"].unit(), units::dimensionless);
}

TYPED_TEST(DataProxyTest, unit_access_fails_without_values) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);
  d.setSparseCoord("sparse", makeSparseVariable<double>({}, Dim::X));
  EXPECT_ANY_THROW(d_ref["sparse"].unit());
}

TYPED_TEST(DataProxyTest, coords) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);
  const auto var = makeVariable<double>({Dim::X, 3});
  d.setCoord(Dim::X, var);
  d.setValues("a", var);

  ASSERT_NO_THROW(d_ref["a"].coords());
  ASSERT_EQ(d_ref["a"].coords().size(), 1);
  ASSERT_NO_THROW(d_ref["a"].coords()[Dim::X]);
  ASSERT_EQ(d_ref["a"].coords()[Dim::X], var);
}

TYPED_TEST(DataProxyTest, coords_sparse) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);
  const auto var = makeSparseVariable<double>({Dim::X, 3}, Dim::Y);
  d.setSparseCoord("a", var);

  ASSERT_NO_THROW(d_ref["a"].coords());
  ASSERT_EQ(d_ref["a"].coords().size(), 1);
  ASSERT_NO_THROW(d_ref["a"].coords()[Dim::Y]);
  ASSERT_EQ(d_ref["a"].coords()[Dim::Y], var);
}

TYPED_TEST(DataProxyTest, coords_sparse_shadow) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
  const auto sparse = makeSparseVariable<double>({Dim::X, 3}, Dim::Y);
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setSparseCoord("a", sparse);

  ASSERT_NO_THROW(d_ref["a"].coords());
  ASSERT_EQ(d_ref["a"].coords().size(), 2);
  ASSERT_NO_THROW(d_ref["a"].coords()[Dim::X]);
  ASSERT_NO_THROW(d_ref["a"].coords()[Dim::Y]);
  ASSERT_EQ(d_ref["a"].coords()[Dim::X], x);
  ASSERT_NE(d_ref["a"].coords()[Dim::Y], y);
  ASSERT_EQ(d_ref["a"].coords()[Dim::Y], sparse);
}

TYPED_TEST(DataProxyTest, coords_sparse_shadow_even_if_no_coord) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
  const auto sparse = makeSparseVariable<double>({Dim::X, 3}, Dim::Y);
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setValues("a", sparse);

  ASSERT_NO_THROW(d_ref["a"].coords());
  // Dim::Y is sparse, so the global (non-sparse) Y coordinate does not make
  // sense and is thus hidden.
  ASSERT_EQ(d_ref["a"].coords().size(), 1);
  ASSERT_NO_THROW(d_ref["a"].coords()[Dim::X]);
  ASSERT_ANY_THROW(d_ref["a"].coords()[Dim::Y]);
  ASSERT_EQ(d_ref["a"].coords()[Dim::X], x);
}

TYPED_TEST(DataProxyTest, coords_contains_only_relevant) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
  const auto var = makeVariable<double>({Dim::X, 3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setValues("a", var);
  const auto coords = d_ref["a"].coords();

  ASSERT_EQ(coords.size(), 1);
  ASSERT_NO_THROW(coords[Dim::X]);
  ASSERT_EQ(coords[Dim::X], x);
}

TYPED_TEST(DataProxyTest, coords_contains_only_relevant_2d_dropped) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
  const auto var = makeVariable<double>({Dim::X, 3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setValues("a", var);
  const auto coords = d_ref["a"].coords();

  ASSERT_EQ(coords.size(), 1);
  ASSERT_NO_THROW(coords[Dim::X]);
  ASSERT_EQ(coords[Dim::X], x);
}

TYPED_TEST(DataProxyTest, coords_contains_only_relevant_2d_dropped_relevant) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);
  const auto x = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
  const auto y = makeVariable<double>({Dim::Y, 3});
  const auto var = makeVariable<double>({Dim::X, 3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setValues("a", var);
  const auto coords = d_ref["a"].coords();

  // This is a very special case which is probably unlikely to occur in
  // practice. If the coordinate depends on extra dimensions and the data is
  // not, it implies that the coordinate cannot be for this data item, so it is
  // dropped.
  ASSERT_EQ(coords.size(), 0);
}

TYPED_TEST(DataProxyTest, hasValues_hasVariances) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);
  const auto var = makeVariable<double>({});

  d.setValues("a", var);
  d.setValues("b", var);
  d.setVariances("b", var);

  ASSERT_TRUE(d_ref["a"].hasValues());
  ASSERT_FALSE(d_ref["a"].hasVariances());

  ASSERT_TRUE(d_ref["b"].hasValues());
  ASSERT_TRUE(d_ref["b"].hasVariances());
}

TYPED_TEST(DataProxyTest, values_variances) {
  next::Dataset d;
  typename TestFixture::proxy_type &d_ref(d);
  const auto var = makeVariable<double>({Dim::X, 2}, {1, 2});
  d.setValues("a", var);
  d.setVariances("a", var);

  ASSERT_EQ(d_ref["a"].values(), var);
  ASSERT_EQ(d_ref["a"].variances(), var);
  ASSERT_TRUE(equals(d_ref["a"].template values<double>(), {1, 2}));
  ASSERT_TRUE(equals(d_ref["a"].template variances<double>(), {1, 2}));
  ASSERT_ANY_THROW(d_ref["a"].template values<float>());
  ASSERT_ANY_THROW(d_ref["a"].template variances<float>());
}

template <typename T>
class DataProxy_values_xy_coords_x_y : public ::testing::Test {
protected:
  T dataset = []() {
    next::Dataset d;
    const auto x = makeVariable<double>({Dim::X, 4}, {1, 2, 3, 4});
    const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
    const auto var = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 4}});
    d.setCoord(Dim::X, x);
    d.setCoord(Dim::Y, y);
    d.setValues("a", var);
    return d;
  }();
};

using DataProxyTypes = ::testing::Types<next::Dataset, const next::Dataset>;
TYPED_TEST_CASE(DataProxy_values_xy_coords_x_y, DataProxyTypes);

TYPED_TEST(DataProxy_values_xy_coords_x_y, slice_single) {
  auto &d = TestFixture::dataset;

  ASSERT_NO_THROW(d["a"].slice({Dim::X, 1}));
  const auto sliceX = d["a"].slice({Dim::X, 1});
  ASSERT_EQ(sliceX.dims(), Dimensions({Dim::Y, 3}));
}

TYPED_TEST(DataProxy_values_xy_coords_x_y, slice_length_1) {
  auto &d = TestFixture::dataset;

  ASSERT_NO_THROW(d["a"].slice({Dim::X, 1, 2}));
  const auto sliceX = d["a"].slice({Dim::X, 1, 2});
  ASSERT_EQ(sliceX.dims(), Dimensions({{Dim::Y, 3}, {Dim::X, 1}}));
}

TYPED_TEST(DataProxy_values_xy_coords_x_y, slice) {
  auto &d = TestFixture::dataset;

  ASSERT_NO_THROW(d["a"].slice({Dim::X, 1, 3}));
  const auto sliceX = d["a"].slice({Dim::X, 1, 3});
  ASSERT_EQ(sliceX.dims(), Dimensions({{Dim::Y, 3}, {Dim::X, 2}}));
}

TYPED_TEST(DataProxy_values_xy_coords_x_y, slice_single_coords) {
  auto &d = TestFixture::dataset;
  const auto slice = d["a"].slice({Dim::X, 1});
  const auto coords = slice.coords();

  ASSERT_EQ(coords.size(), 1);
  ASSERT_EQ(coords[Dim::Y].dims(), Dimensions({Dim::Y, 3}));
}

TYPED_TEST(DataProxy_values_xy_coords_x_y, slice_length_1_coords) {
  auto &d = TestFixture::dataset;
  const auto slice = d["a"].slice({Dim::X, 1, 2});
  const auto coords = slice.coords();

  ASSERT_EQ(coords.size(), 2);
  ASSERT_EQ(coords[Dim::X].dims(), Dimensions({Dim::X, 1}));
  ASSERT_EQ(coords[Dim::Y].dims(), Dimensions({Dim::Y, 3}));
}

TYPED_TEST(DataProxy_values_xy_coords_x_y, slice_coords) {
  auto &d = TestFixture::dataset;
  const auto slice = d["a"].slice({Dim::X, 1, 3});
  const auto coords = slice.coords();

  ASSERT_EQ(coords.size(), 2);
  ASSERT_EQ(coords[Dim::X].dims(), Dimensions({Dim::X, 2}));
  ASSERT_EQ(coords[Dim::Y].dims(), Dimensions({Dim::Y, 3}));
}

template <typename T>
class DataProxy_values_xy_coords_xbins_y : public ::testing::Test {
protected:
  T dataset = []() {
    next::Dataset d;
    const auto x = makeVariable<double>({Dim::X, 4}, {1, 2, 3, 4});
    const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
    const auto var = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    d.setCoord(Dim::X, x);
    d.setCoord(Dim::Y, y);
    d.setValues("a", var);
    return d;
  }();
};

using DataProxyTypes = ::testing::Types<next::Dataset, const next::Dataset>;
TYPED_TEST_CASE(DataProxy_values_xy_coords_xbins_y, DataProxyTypes);

TYPED_TEST(DataProxy_values_xy_coords_xbins_y, slice_single) {
  auto &d = TestFixture::dataset;

  ASSERT_NO_THROW(d["a"].slice({Dim::X, 1}));
  const auto sliceX = d["a"].slice({Dim::X, 1});
  ASSERT_EQ(sliceX.dims(), Dimensions({Dim::Y, 3}));
}

TYPED_TEST(DataProxy_values_xy_coords_xbins_y, slice_length_1) {
  auto &d = TestFixture::dataset;

  ASSERT_NO_THROW(d["a"].slice({Dim::X, 1, 2}));
  const auto sliceX = d["a"].slice({Dim::X, 1, 2});
  ASSERT_EQ(sliceX.dims(), Dimensions({{Dim::Y, 3}, {Dim::X, 1}}));
}

TYPED_TEST(DataProxy_values_xy_coords_xbins_y, slice) {
  auto &d = TestFixture::dataset;

  ASSERT_NO_THROW(d["a"].slice({Dim::X, 1, 3}));
  const auto sliceX = d["a"].slice({Dim::X, 1, 3});
  ASSERT_EQ(sliceX.dims(), Dimensions({{Dim::Y, 3}, {Dim::X, 2}}));
}

TYPED_TEST(DataProxy_values_xy_coords_xbins_y, slice_single_coords) {
  auto &d = TestFixture::dataset;
  const auto slice = d["a"].slice({Dim::X, 1});
  const auto coords = slice.coords();

  ASSERT_EQ(coords.size(), 1);
  ASSERT_EQ(coords[Dim::Y].dims(), Dimensions({Dim::Y, 3}));
}

TYPED_TEST(DataProxy_values_xy_coords_xbins_y, slice_length_1_coords) {
  auto &d = TestFixture::dataset;
  const auto slice = d["a"].slice({Dim::X, 1, 2});
  const auto coords = slice.coords();

  ASSERT_EQ(coords.size(), 2);
  ASSERT_EQ(coords[Dim::X].dims(), Dimensions({Dim::X, 2}));
  ASSERT_EQ(coords[Dim::Y].dims(), Dimensions({Dim::Y, 3}));
}

TYPED_TEST(DataProxy_values_xy_coords_xbins_y, slice_coords) {
  auto &d = TestFixture::dataset;
  const auto slice = d["a"].slice({Dim::X, 1, 3});
  const auto coords = slice.coords();

  ASSERT_EQ(coords.size(), 2);
  ASSERT_EQ(coords[Dim::X].dims(), Dimensions({Dim::X, 3}));
  ASSERT_EQ(coords[Dim::Y].dims(), Dimensions({Dim::Y, 3}));
}
