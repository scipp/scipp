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
#include "scipp/dataset/unaligned.h"
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
  DataArray_comparison_operators()
      : default_event_weights(makeVariable<double>(
            Dims{Dim::Y, Dim::Z}, Shape{3l, 2l}, Values{1, 1, 1, 1, 1, 1},
            Variances{1, 1, 1, 1, 1, 1})),
        events_variable(makeVariable<event_list<double>>(Dims{Dim::Y, Dim::Z},
                                                         Shape{3, 2})) {
    dataset.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{4}));
    dataset.setCoord(Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{3}));

    dataset.setCoord(Dim("labels"), makeVariable<int>(Dims{Dim::X}, Shape{4}));
    dataset.setMask("mask", makeVariable<bool>(Dims{Dim::X}, Shape{4}));

    dataset.setAttr("global_attr", makeVariable<int>(Values{int{}}));

    dataset.setData("val_and_var",
                    makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 4},
                                         Values(12), Variances(12)));
    dataset.setAttr("val_and_var", "attr", makeVariable<int>(Values{int{}}));

    dataset.setData("val", makeVariable<double>(Dims{Dim::X}, Shape{4}));
    dataset.setAttr("val", "attr", makeVariable<int>(Values{int{}}));
  }

  Dataset dataset;
  Variable default_event_weights;
  Variable events_variable;
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
  d.setMask(name,
            makeVariable<T>(Dimensions(dims), units::Unit(unit), Values(data)));
  d.setData("", makeVariable<T>(Dimensions(dims)));
  return DataArray(d[""]);
}

template <class T, class T2>
auto make_1_attr(const std::string &name, const Dimensions &dims,
                 const units::Unit unit,
                 const std::initializer_list<T2> &data) {
  Dataset d;
  d.setData("", makeVariable<T>(Dimensions(dims)));
  d.setAttr("", name,
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
  extra.setMask("extra", makeVariable<bool>(Values{false}));
  for (const auto &a : extra)
    expect_ne(a, dataset[a.name()]);
}

TEST_F(DataArray_comparison_operators, extra_attr) {
  auto extra = dataset;
  for (const auto &a : extra) {
    extra.setAttr(a.name(), "extra", makeVariable<double>(Values{0.0}));
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
  auto a = Dataset();
  auto b = Dataset();
  a.setCoord(Dim::X, dataset.coords()[Dim::X]);
  a.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  b.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  b.setCoord(Dim::X, dataset.coords()[Dim::X]);
  for (const auto &a_ : a)
    expect_ne(a_, b[a_.name()]);
}

TEST_F(DataArray_comparison_operators, different_label_insertion_order) {
  auto a = Dataset();
  auto b = Dataset();
  a.setCoord(Dim("x"), dataset.coords()[Dim::X]);
  a.setCoord(Dim("y"), dataset.coords()[Dim::Y]);
  b.setCoord(Dim("y"), dataset.coords()[Dim::Y]);
  b.setCoord(Dim("x"), dataset.coords()[Dim::X]);
  for (const auto &a_ : a)
    expect_ne(a_, b[a_.name()]);
}

TEST_F(DataArray_comparison_operators, different_attr_insertion_order) {
  auto a = Dataset();
  auto b = Dataset();
  a.setAttr("x", dataset.coords()[Dim::X]);
  a.setAttr("y", dataset.coords()[Dim::Y]);
  b.setAttr("y", dataset.coords()[Dim::Y]);
  b.setAttr("x", dataset.coords()[Dim::X]);
  for (const auto &a_ : a)
    expect_ne(a_, b[a_.name()]);
}

TEST_F(DataArray_comparison_operators, with_events_dimension_data) {
  // a and b same, c different number of events values
  auto a = Dataset();
  auto data = makeVariable<event_list<double>>(Dims{}, Shape{});
  const std::string var_name = "test_var";
  data.values<event_list<double>>()[0] = {1, 2, 3};
  a.setData(var_name, data);
  auto b = Dataset();
  b.setData(var_name, data);
  expect_eq(a[var_name], b[var_name]);
  data.values<event_list<double>>()[0] = {2, 3, 4};
  auto c = Dataset();
  c.setData(var_name, data);
  expect_ne(a[var_name], c[var_name]);
  expect_ne(b[var_name], c[var_name]);
}

class DataArray_comparison_operators_realigned : public ::testing::Test {
protected:
  Variable ybins{makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{0, 2, 4})};
  Variable zbins{makeVariable<double>(Dims{Dim::Z}, Shape{3}, Values{0, 2, 4})};
  Variable d{makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4})};
  Variable x{makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4})};
  Variable y{makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 1, 3, 3})};
  Variable z{makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 3, 1, 3})};
};

TEST_F(DataArray_comparison_operators_realigned, self) {
  DataArray a(d, {{Dim::X, x}, {Dim::Y, y}, {Dim::Z, z}});
  const auto realigned =
      unaligned::realign(a, {{Dim::Y, ybins}, {Dim::Z, zbins}});
  expect_eq(realigned, realigned);
}

TEST_F(DataArray_comparison_operators_realigned, swapped_dims) {
  DataArray a(d, {{Dim::X, x}, {Dim::Y, y}, {Dim::Z, z}});
  const auto zy = unaligned::realign(a, {{Dim::Y, ybins}, {Dim::Z, zbins}});
  const auto yz = unaligned::realign(a, {{Dim::Z, zbins}, {Dim::Y, ybins}});
  expect_ne(yz, zy);
}

TEST_F(DataArray_comparison_operators_realigned, different_bins) {
  DataArray a(d, {{Dim::X, x}, {Dim::Y, y}, {Dim::Z, z}});
  const auto yz1 = unaligned::realign(a, {{Dim::Y, ybins}, {Dim::Z, zbins}});
  const auto yz2 = unaligned::realign(
      a, {{Dim::Y, ybins}, {Dim::Z, zbins + 0.5 * units::one}});
  expect_ne(yz1, yz2);
}

TEST_F(DataArray_comparison_operators_realigned, different_unaligned_data) {
  DataArray a1(d, {{Dim::X, x}, {Dim::Y, y}, {Dim::Z, z}});
  DataArray a2(d + 0.5 * units::one, {{Dim::X, x}, {Dim::Y, y}, {Dim::Z, z}});
  const auto realigned1 =
      unaligned::realign(a1, {{Dim::Y, ybins}, {Dim::Z, zbins}});
  const auto realigned2 =
      unaligned::realign(a2, {{Dim::Y, ybins}, {Dim::Z, zbins}});
  expect_ne(realigned1, realigned2);
}

TEST_F(DataArray_comparison_operators_realigned, different_unaligned_coord) {
  DataArray a1(d, {{Dim::X, x}, {Dim::Y, y}, {Dim::Z, z}});
  DataArray a2(d, {{Dim::X, x}, {Dim::Y, y + 0.5 * units::one}, {Dim::Z, z}});
  const auto realigned1 =
      unaligned::realign(a1, {{Dim::Y, ybins}, {Dim::Z, zbins}});
  const auto realigned2 =
      unaligned::realign(a2, {{Dim::Y, ybins}, {Dim::Z, zbins}});
  expect_ne(realigned1, realigned2);
}
