// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
//
// The test in this file ensure that comparison operators for DataArray and
// DataArrayConstView are correct. More complex tests should build on the
// assumption that comparison operators are correct.
#include "test_macros.h"
#include <gtest/gtest.h>

#include <numeric>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::dataset;

namespace {
void expect_eq(const DataArrayConstView &a, const DataArrayConstView &b) {
  EXPECT_TRUE(a == b);
  EXPECT_TRUE(b == a);
  EXPECT_FALSE(a != b);
  EXPECT_FALSE(b != a);
}
void expect_ne(const DataArrayConstView &a, const DataArrayConstView &b) {
  EXPECT_TRUE(a != b);
  EXPECT_TRUE(b != a);
  EXPECT_FALSE(a == b);
  EXPECT_FALSE(b == a);
}
} // namespace

class DataArray_comparison_operators : public ::testing::Test {
protected:
  DataArray_comparison_operators() {
    dataset.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{4}));
    dataset.setCoord(Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{3}));

    dataset.setCoord(Dim("labels"), makeVariable<int>(Dims{Dim::X}, Shape{4}));

    dataset.setData("val_and_var",
                    makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 4},
                                         Values(12), Variances(12)));
    dataset.setCoord("val_and_var", Dim("attr"),
                     makeVariable<int>(Values{int{}}));

    dataset.setData("val", makeVariable<double>(Dims{Dim::X}, Shape{4}));
    dataset.setCoord("val", Dim("attr"), makeVariable<int>(Values{int{}}));
    for (const auto &item : {"val_and_var", "val"})
      dataset[item].masks().set("mask",
                                makeVariable<bool>(Dims{Dim::X}, Shape{4}));
  }

  Dataset dataset;
};

template <class T> auto make_values(const Dimensions &dims) {
  Dataset d;
  d.setData("", makeVariable<T>(Dimensions(dims)));
  return DataArray(d[""]);
}

template <class T, class T2>
auto make_1_coord(const Dim dim, const Dimensions &dims, const units::Unit unit,
                  const std::initializer_list<T2> &data) {
  Dataset d;
  d.setCoord(
      dim, makeVariable<T>(Dimensions(dims), units::Unit(unit), Values(data)));
  d.setData("", makeVariable<T>(Dimensions(dims)));
  return DataArray(d[""]);
}

template <class T, class T2>
auto make_1_labels(const std::string &name, const Dimensions &dims,
                   const units::Unit unit,
                   const std::initializer_list<T2> &data) {
  Dataset d;
  d.setCoord(Dim(name), makeVariable<T>(Dimensions(dims), units::Unit(unit),
                                        Values(data)));
  d.setData("", makeVariable<T>(Dimensions(dims)));
  return DataArray(d[""]);
}

template <class T, class T2>
auto make_1_mask(const std::string &name, const Dimensions &dims,
                 const units::Unit unit,
                 const std::initializer_list<T2> &data) {
  Dataset d;
  d.setData("", makeVariable<T>(Dimensions(dims)));
  d[""].masks().set(
      name, makeVariable<T>(Dimensions(dims), units::Unit(unit), Values(data)));
  return DataArray(d[""]);
}

template <class T, class T2>
auto make_1_attr(const std::string &name, const Dimensions &dims,
                 const units::Unit unit,
                 const std::initializer_list<T2> &data) {
  Dataset d;
  d.setData("", makeVariable<T>(Dimensions(dims)));
  d.setCoord(
      "", Dim(name),
      makeVariable<T>(Dimensions(dims), units::Unit(unit), Values(data)));
  return DataArray(d[""]);
}

template <class T, class T2>
auto make_values(const std::string &name, const Dimensions &dims,
                 const units::Unit unit,
                 const std::initializer_list<T2> &data) {
  Dataset d;
  d.setData(name,
            makeVariable<T>(Dimensions(dims), units::Unit(unit), Values(data)));
  return DataArray(d[name]);
}

template <class T, class T2>
auto make_values_and_variances(const std::string &name, const Dimensions &dims,
                               const units::Unit unit,
                               const std::initializer_list<T2> &values,
                               const std::initializer_list<T2> &variances) {
  Dataset d;
  d.setData(name, makeVariable<T>(Dimensions(dims), units::Unit(unit),
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
    DataArray a(item);
    expect_eq(a, a);
  }
}

TEST_F(DataArray_comparison_operators, copy) {
  auto copy = dataset;
  for (const auto &a : copy) {
    expect_eq(a, dataset[a.name()]);
  }
}

TEST_F(DataArray_comparison_operators, extra_coord) {
  auto extra = dataset;
  extra.setCoord(Dim::Z, makeVariable<double>(Values{0.0}));
  for (const auto &a : extra)
    expect_ne(a, dataset[a.name()]);
}

TEST_F(DataArray_comparison_operators, extra_labels) {
  auto extra = dataset;
  extra.setCoord(Dim("extra"), makeVariable<double>(Values{0.0}));
  for (const auto &a : extra)
    expect_ne(a, dataset[a.name()]);
}

TEST_F(DataArray_comparison_operators, extra_mask) {
  auto extra = dataset;
  for (const auto &a : extra) {
    a.masks().set("extra", makeVariable<bool>(Values{false}));
    expect_ne(a, dataset[a.name()]);
  }
}

TEST_F(DataArray_comparison_operators, extra_attr) {
  auto extra = dataset;
  for (const auto &a : extra) {
    extra.setCoord(a.name(), Dim("extra"), makeVariable<double>(Values{0.0}));
    expect_ne(a, dataset[a.name()]);
  }
}

TEST_F(DataArray_comparison_operators, extra_variance) {
  auto extra = dataset;
  extra.setData("val", makeVariable<double>(Dimensions{Dim::X, 4}, Values(4),
                                            Variances(4)));
  expect_ne(extra["val"], dataset["val"]);
}

TEST_F(DataArray_comparison_operators, different_coord_insertion_order) {
  const auto var = makeVariable<double>(Values{1.0});
  auto a = DataArray(var);
  auto b = DataArray(var);
  a.coords().set(Dim::X, dataset.coords()[Dim::X]);
  a.coords().set(Dim::Y, dataset.coords()[Dim::Y]);
  b.coords().set(Dim::Y, dataset.coords()[Dim::Y]);
  b.coords().set(Dim::X, dataset.coords()[Dim::X]);
  expect_eq(a, b);
}

TEST_F(DataArray_comparison_operators, different_attr_insertion_order) {
  auto a = Dataset();
  auto b = Dataset();
  const auto var = makeVariable<double>(Values{1.0});
  a.setData("item", var);
  b.setData("item", var);
  a["item"].coords().set(Dim::X, dataset.coords()[Dim::X]);
  a["item"].coords().set(Dim::Y, dataset.coords()[Dim::Y]);
  b["item"].coords().set(Dim::Y, dataset.coords()[Dim::Y]);
  b["item"].coords().set(Dim::X, dataset.coords()[Dim::X]);
  for (const auto &a_ : a)
    expect_eq(a_, b[a_.name()]);
}
