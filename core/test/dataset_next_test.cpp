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
