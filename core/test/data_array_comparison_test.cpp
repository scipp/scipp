// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
//
// The test in this file ensure that comparison operators for DataArray and
// DataConstProxy are correct. More complex tests should build on the assumption
// that comparison operators are correct.
#include "test_macros.h"
#include <gtest/gtest.h>

#include <numeric>

#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"

using namespace scipp;
using namespace scipp::core;

class DataArray_comparison_operators : public ::testing::Test {
protected:
  DataArray_comparison_operators()
      : sparse_variable(makeVariable<double>({Dim::Y, Dim::Z, Dim::X},
                                             {3, 2, Dimensions::Sparse})) {
    dataset.setCoord(Dim::X, createVariable<double>(Dims{Dim::X}, Shape{4}));
    dataset.setCoord(Dim::Y, createVariable<double>(Dims{Dim::Y}, Shape{3}));

    dataset.setLabels("labels", createVariable<int>(Dims{Dim::X}, Shape{4}));
    dataset.setMask("mask", createVariable<bool>(Dims{Dim::X}, Shape{4}));

    dataset.setAttr("global_attr", makeVariable<int>({}));

    auto vector = std::vector<double>(12);
    dataset.setData(
        "val_and_var",
        createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 4},
                               Values(vector.begin(), vector.end()),
                               Variances(vector.begin(), vector.end())));
    dataset.setAttr("val_and_var", "attr", makeVariable<int>({}));

    dataset.setData("val", createVariable<double>(Dims{Dim::X}, Shape{4}));
    dataset.setAttr("val", "attr", makeVariable<int>({}));

    dataset.setSparseCoord("sparse_coord", sparse_variable);
    dataset.setAttr("sparse_coord", "attr", makeVariable<int>({}));
    dataset.setData("sparse_coord_and_val", sparse_variable);
    dataset.setSparseCoord("sparse_coord_and_val", sparse_variable);
    dataset.setAttr("sparse_coord_and_val", "attr", makeVariable<int>({}));
  }
  void expect_eq(const DataConstProxy &a, const DataConstProxy &b) const {
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(b != a);
  }
  void expect_ne(const DataConstProxy &a, const DataConstProxy &b) const {
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
  }

  Dataset dataset;
  Variable sparse_variable;
};

template <class T> auto make_values(const Dimensions &dims) {
  Dataset d;
  d.setData("", createVariable<T>(Dimensions(dims)));
  return DataArray(d[""]);
}

template <class T, class T2>
auto make_1_coord(const Dim dim, const Dimensions &dims, const units::Unit unit,
                  const std::initializer_list<T2> &data) {
  Dataset d;
  d.setCoord(dim, createVariable<T>(Dimensions(dims), units::Unit(unit),
                                    Values(data)));
  d.setData("", createVariable<T>(Dimensions(dims)));
  return DataArray(d[""]);
}

template <class T, class T2>
auto make_1_labels(const std::string &name, const Dimensions &dims,
                   const units::Unit unit,
                   const std::initializer_list<T2> &data) {
  Dataset d;
  d.setLabels(name, createVariable<T>(Dimensions(dims), units::Unit(unit),
                                      Values(data)));
  d.setData("", createVariable<T>(Dimensions(dims)));
  return DataArray(d[""]);
}

template <class T, class T2>
auto make_1_mask(const std::string &name, const Dimensions &dims,
                 const units::Unit unit,
                 const std::initializer_list<T2> &data) {
  Dataset d;
  d.setMask(name, createVariable<T>(Dimensions(dims), units::Unit(unit),
                                    Values(data)));
  d.setData("", createVariable<T>(Dimensions(dims)));
  return DataArray(d[""]);
}

template <class T, class T2>
auto make_1_attr(const std::string &name, const Dimensions &dims,
                 const units::Unit unit,
                 const std::initializer_list<T2> &data) {
  Dataset d;
  d.setData("", createVariable<T>(Dimensions(dims)));
  d.setAttr(
      "", name,
      createVariable<T>(Dimensions(dims), units::Unit(unit), Values(data)));
  return DataArray(d[""]);
}

template <class T, class T2>
auto make_values(const std::string &name, const Dimensions &dims,
                 const units::Unit unit,
                 const std::initializer_list<T2> &data) {
  Dataset d;
  d.setData(name, createVariable<T>(Dimensions(dims), units::Unit(unit),
                                    Values(data)));
  return DataArray(d[name]);
}

template <class T, class T2>
auto make_values_and_variances(const std::string &name, const Dimensions &dims,
                               const units::Unit unit,
                               const std::initializer_list<T2> &values,
                               const std::initializer_list<T2> &variances) {
  Dataset d;
  d.setData(name, createVariable<T>(Dimensions(dims), units::Unit(unit),
                                    Values(values), Variances(variances)));
  return DataArray(d[name]);
}

// Baseline checks: Does data-array comparison pick up arbitrary mismatch of
// individual items? Strictly speaking many of these are just retesting the
// comparison of Variable, but it ensures that the content is actually compared
// and thus serves as a baseline for the follow-up tests.
TEST_F(DataArray_comparison_operators, single_coord) {
  auto a =
      make_1_coord<double>(Dim::X, {Dim::X, 3}, units::m, {false, true, false});
  expect_eq(a, a);
  expect_ne(a, make_values<double>({Dim::X, 3}));
  expect_ne(a, make_1_coord<float>(Dim::X, {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_1_coord<double>(Dim::Y, {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_1_coord<double>(Dim::X, {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_1_coord<double>(Dim::X, {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(a, make_1_coord<double>(Dim::X, {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(a, make_1_coord<double>(Dim::X, {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(DataArray_comparison_operators, single_labels) {
  auto a = make_1_labels<double>("a", {Dim::X, 3}, units::m, {1, 2, 3});
  expect_eq(a, a);
  expect_ne(a, make_values<double>({Dim::X, 3}));
  expect_ne(a, make_1_labels<float>("a", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_1_labels<double>("b", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_1_labels<double>("a", {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_1_labels<double>("a", {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(a, make_1_labels<double>("a", {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(a, make_1_labels<double>("a", {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(DataArray_comparison_operators, single_mask) {
  auto a = make_1_mask<bool>("a", {Dim::X, 3}, units::m, {true, false, true});
  expect_eq(a, a);
  expect_ne(a, make_values<bool>({Dim::X, 3}));
  expect_ne(a,
            make_1_mask<bool>("b", {Dim::X, 3}, units::m, {true, false, true}));
  expect_ne(a,
            make_1_mask<bool>("a", {Dim::Y, 3}, units::m, {true, false, true}));
  expect_ne(a, make_1_mask<bool>("a", {Dim::X, 2}, units::m, {true, false}));
  expect_ne(a,
            make_1_mask<bool>("a", {Dim::X, 3}, units::s, {true, false, true}));
  expect_ne(
      a, make_1_mask<bool>("a", {Dim::X, 3}, units::m, {false, false, false}));
}

TEST_F(DataArray_comparison_operators, single_attr) {
  auto a = make_1_attr<double>("a", {Dim::X, 3}, units::m, {1, 2, 3});
  expect_eq(a, a);
  expect_ne(a, make_values<double>({Dim::X, 3}));
  expect_ne(a, make_1_attr<float>("a", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_1_attr<double>("b", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_1_attr<double>("a", {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_1_attr<double>("a", {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(a, make_1_attr<double>("a", {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(a, make_1_attr<double>("a", {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(DataArray_comparison_operators, single_values) {
  auto a = make_values<double>("a", {Dim::X, 3}, units::m, {1, 2, 3});
  expect_eq(a, a);
  // Name of DataArray is ignored in comparison.
  expect_eq(a, make_values<double>("b", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_values<double>({Dim::X, 3}));
  expect_ne(a, make_values<float>("a", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_values<double>("a", {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(a, make_values<double>("a", {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(a, make_values<double>("a", {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(a, make_values<double>("a", {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(DataArray_comparison_operators, single_values_and_variances) {
  auto a = make_values_and_variances<double>("a", {Dim::X, 3}, units::m,
                                             {1, 2, 3}, {4, 5, 6});
  expect_eq(a, a);
  // Name of DataArray is ignored in comparison.
  expect_eq(a, make_values_and_variances<double>("b", {Dim::X, 3}, units::m,
                                                 {1, 2, 3}, {4, 5, 6}));
  expect_ne(a, make_values_and_variances<float>("a", {Dim::X, 3}, units::m,
                                                {1, 2, 3}, {4, 5, 6}));
  expect_ne(a, make_values_and_variances<double>("a", {Dim::Y, 3}, units::m,
                                                 {1, 2, 3}, {4, 5, 6}));
  expect_ne(a, make_values_and_variances<double>("a", {Dim::X, 2}, units::m,
                                                 {1, 2}, {4, 5}));
  expect_ne(a, make_values_and_variances<double>("a", {Dim::X, 3}, units::s,
                                                 {1, 2, 3}, {4, 5, 6}));
  expect_ne(a, make_values_and_variances<double>("a", {Dim::X, 3}, units::m,
                                                 {1, 2, 4}, {4, 5, 6}));
  expect_ne(a, make_values_and_variances<double>("a", {Dim::X, 3}, units::m,
                                                 {1, 2, 3}, {4, 5, 7}));
}
// End baseline checks.

TEST_F(DataArray_comparison_operators, self) {
  for (const auto item : dataset) {
    DataArray a(item.second);
    expect_eq(a, a);
  }
}

TEST_F(DataArray_comparison_operators, copy) {
  auto copy = dataset;
  for (const auto [name, a] : copy) {
    expect_eq(a, dataset[name]);
  }
}

TEST_F(DataArray_comparison_operators, extra_coord) {
  auto extra = dataset;
  extra.setCoord(Dim::Z, makeVariable<double>(0.0));
  for (const auto [name, a] : extra)
    expect_ne(a, dataset[name]);
}

TEST_F(DataArray_comparison_operators, extra_labels) {
  auto extra = dataset;
  extra.setLabels("extra", makeVariable<double>(0.0));
  for (const auto [name, a] : extra)
    expect_ne(a, dataset[name]);
}

TEST_F(DataArray_comparison_operators, extra_mask) {
  auto extra = dataset;
  extra.setMask("extra", makeVariable<bool>(false));
  for (const auto [name, a] : extra)
    expect_ne(a, dataset[name]);
}

TEST_F(DataArray_comparison_operators, extra_attr) {
  auto extra = dataset;
  for (const auto [name, a] : extra) {
    extra.setAttr(name, "extra", makeVariable<double>(0.0));
    expect_ne(a, dataset[name]);
  }
}

TEST_F(DataArray_comparison_operators, extra_variance) {
  auto extra = dataset;
  auto vector = std::vector<double>(4);
  extra.setData(
      "val", createVariable<double>(Dimensions{Dim::X, 4},
                                    Values(vector.begin(), vector.end()),
                                    Variances(vector.begin(), vector.end())));
  expect_ne(extra["val"], dataset["val"]);
}

TEST_F(DataArray_comparison_operators, extra_sparse_values) {
  auto extra = dataset;
  extra.setData("sparse_coord", sparse_variable);
  expect_ne(extra["sparse_coord"], dataset["sparse_coord"]);
}

TEST_F(DataArray_comparison_operators, extra_sparse_label) {
  auto extra = dataset;
  extra.setSparseLabels("sparse_coord_and_val", "extra", sparse_variable);
  expect_ne(extra["sparse_coord_and_val"], dataset["sparse_coord_and_val"]);
}

TEST_F(DataArray_comparison_operators, different_coord_insertion_order) {
  auto a = Dataset();
  auto b = Dataset();
  a.setCoord(Dim::X, dataset.coords()[Dim::X]);
  a.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  b.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  b.setCoord(Dim::X, dataset.coords()[Dim::X]);
  for (const auto [name, a_] : a)
    expect_ne(a_, b[name]);
}

TEST_F(DataArray_comparison_operators, different_label_insertion_order) {
  auto a = Dataset();
  auto b = Dataset();
  a.setLabels("x", dataset.coords()[Dim::X]);
  a.setLabels("y", dataset.coords()[Dim::Y]);
  b.setLabels("y", dataset.coords()[Dim::Y]);
  b.setLabels("x", dataset.coords()[Dim::X]);
  for (const auto [name, a_] : a)
    expect_ne(a_, b[name]);
}

TEST_F(DataArray_comparison_operators, different_attr_insertion_order) {
  auto a = Dataset();
  auto b = Dataset();
  a.setAttr("x", dataset.coords()[Dim::X]);
  a.setAttr("y", dataset.coords()[Dim::Y]);
  b.setAttr("y", dataset.coords()[Dim::Y]);
  b.setAttr("x", dataset.coords()[Dim::X]);
  for (const auto [name, a_] : a)
    expect_ne(a_, b[name]);
}

TEST_F(DataArray_comparison_operators, with_sparse_dimension_data) {
  // a and b same, c different number of sparse values
  auto a = Dataset();
  auto data = createVariable<double>(Dims{Dim::X}, Shape{Dimensions::Sparse});
  const std::string var_name = "test_var";
  data.sparseValues<double>()[0] = {1, 2, 3};
  a.setData(var_name, data);
  auto b = Dataset();
  b.setData(var_name, data);
  expect_eq(a[var_name], b[var_name]);
  data.sparseValues<double>()[0] = {2, 3, 4};
  auto c = Dataset();
  c.setData(var_name, data);
  expect_ne(a[var_name], c[var_name]);
  expect_ne(b[var_name], c[var_name]);
}
