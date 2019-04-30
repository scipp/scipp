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

TEST(DataConstProxy, isSparse_sparseDim) {
  next::Dataset d;

  d.setValues("dense", makeVariable<double>({}));
  ASSERT_FALSE(d["dense"].isSparse());
  ASSERT_EQ(d["dense"].sparseDim(), Dim::Invalid);

  d.setValues("sparse_data", makeSparseVariable<double>({}, Dim::X));
  ASSERT_TRUE(d["sparse_data"].isSparse());
  ASSERT_EQ(d["sparse_data"].sparseDim(), Dim::X);

  d.setSparseCoord("sparse_coord", makeSparseVariable<double>({}, Dim::X));
  ASSERT_TRUE(d["sparse_coord"].isSparse());
  ASSERT_EQ(d["sparse_coord"].sparseDim(), Dim::X);
}

TEST(DataConstProxy, dims_shape) {
  next::Dataset d;

  d.setValues("dense", makeVariable<double>({{Dim::X, 1}, {Dim::Y, 2}}));
  ASSERT_TRUE(equals(d["dense"].dims(), {Dim::X, Dim::Y}));
  ASSERT_TRUE(equals(d["dense"].shape(), {1, 2}));

  // Sparse dimension is currently not included in dims(). It is unclear whether
  // this is the right choice. An unfinished idea involves returning
  // std::tuple<std::span<const Dim>, std::optional<Dim>> instead, using `auto [
  // dims, sparse ] = data.dims();`.
  d.setValues("sparse_data",
              makeSparseVariable<double>({{Dim::X, 1}, {Dim::Y, 2}}, Dim::Z));
  ASSERT_TRUE(equals(d["sparse_data"].dims(), {Dim::X, Dim::Y}));
  ASSERT_TRUE(equals(d["sparse_data"].shape(), {1, 2}));

  d.setSparseCoord("sparse_coord", makeSparseVariable<double>(
                                       {{Dim::X, 1}, {Dim::Y, 2}}, Dim::Z));
  ASSERT_TRUE(equals(d["sparse_coord"].dims(), {Dim::X, Dim::Y}));
  ASSERT_TRUE(equals(d["sparse_coord"].shape(), {1, 2}));
}

TEST(DataConstProxy, unit) {
  next::Dataset d;

  d.setValues("dense", makeVariable<double>({}));
  EXPECT_EQ(d["dense"].unit(), units::dimensionless);
}

TEST(DataConstProxy, unit_access_fails_without_values) {
  next::Dataset d;
  d.setSparseCoord("sparse", makeSparseVariable<double>({}, Dim::X));
  EXPECT_ANY_THROW(d["sparse"].unit());
}

TEST(DataConstProxy, coords) {
  next::Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});
  d.setCoord(Dim::X, var);
  d.setValues("a", var);

  ASSERT_NO_THROW(d["a"].coords());
  ASSERT_EQ(d["a"].coords().size(), 1);
  ASSERT_NO_THROW(d["a"].coords()[Dim::X]);
  ASSERT_EQ(d["a"].coords()[Dim::X], var);
}

TEST(DataConstProxy, coords_sparse) {
  next::Dataset d;
  const auto var = makeSparseVariable<double>({Dim::X, 3}, Dim::Y);
  d.setSparseCoord("a", var);

  ASSERT_NO_THROW(d["a"].coords());
  ASSERT_EQ(d["a"].coords().size(), 1);
  ASSERT_NO_THROW(d["a"].coords()[Dim::Y]);
  ASSERT_EQ(d["a"].coords()[Dim::Y], var);
}

TEST(DataConstProxy, coords_sparse_shadow) {
  next::Dataset d;
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
  const auto sparse = makeSparseVariable<double>({Dim::X, 3}, Dim::Y);
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setSparseCoord("a", sparse);

  ASSERT_NO_THROW(d["a"].coords());
  ASSERT_EQ(d["a"].coords().size(), 2);
  ASSERT_NO_THROW(d["a"].coords()[Dim::X]);
  ASSERT_NO_THROW(d["a"].coords()[Dim::Y]);
  ASSERT_EQ(d["a"].coords()[Dim::X], x);
  ASSERT_NE(d["a"].coords()[Dim::Y], y);
  ASSERT_EQ(d["a"].coords()[Dim::Y], sparse);
}

TEST(DataConstProxy, coords_sparse_shadow_even_if_no_coord) {
  next::Dataset d;
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
  const auto sparse = makeSparseVariable<double>({Dim::X, 3}, Dim::Y);
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setValues("a", sparse);

  ASSERT_NO_THROW(d["a"].coords());
  // Dim::Y is sparse, so the global (non-sparse) Y coordinate does not make
  // sense and is thus hidden.
  ASSERT_EQ(d["a"].coords().size(), 1);
  ASSERT_NO_THROW(d["a"].coords()[Dim::X]);
  ASSERT_ANY_THROW(d["a"].coords()[Dim::Y]);
  ASSERT_EQ(d["a"].coords()[Dim::X], x);
}

TEST(DataConstProxy, hasValues_hasVariances) {
  next::Dataset d;
  const auto var = makeVariable<double>({});

  d.setValues("a", var);
  d.setValues("b", var);
  d.setVariances("b", var);

  ASSERT_TRUE(d["a"].hasValues());
  ASSERT_FALSE(d["a"].hasVariances());

  ASSERT_TRUE(d["b"].hasValues());
  ASSERT_TRUE(d["b"].hasVariances());
}

TEST(DataConstProxy, values_variances) {
  next::Dataset d;
  const auto var = makeVariable<double>({Dim::X, 2}, {1, 2});
  d.setValues("a", var);
  d.setVariances("a", var);

  ASSERT_EQ(d["a"].values(), var);
  ASSERT_EQ(d["a"].variances(), var);
  ASSERT_TRUE(equals(d["a"].values<double>(), {1, 2}));
  ASSERT_TRUE(equals(d["a"].variances<double>(), {1, 2}));
  ASSERT_ANY_THROW(d["a"].values<float>());
  ASSERT_ANY_THROW(d["a"].variances<float>());
}
