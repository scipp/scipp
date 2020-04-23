// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
//
// The test in this file ensure that comparison operators for Dataset and
// DatasetConstView are correct. More complex tests should build on the
// assumption that comparison operators are correct.
#include "test_macros.h"
#include <gtest/gtest.h>

#include <numeric>

#include "dataset_test_common.h"
#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"

using namespace scipp;
using namespace scipp::dataset;

class Dataset_comparison_operators : public ::testing::Test {
private:
  template <class A, class B>
  void expect_eq_impl(const A &a, const B &b) const {
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(b != a);
  }
  template <class A, class B>
  void expect_ne_impl(const A &a, const B &b) const {
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
  }

protected:
  Dataset_comparison_operators()
      : events_variable(makeVariable<event_list<double>>(Dims{Dim::Y, Dim::Z},
                                                         Shape{3, 2})) {
    dataset.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{4}));
    dataset.setCoord(Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{3}));

    dataset.setCoord(Dim("labels"), makeVariable<int>(Dims{Dim::X}, Shape{4}));

    dataset.setAttr("attr", makeVariable<int>(Values{int{}}));

    dataset.setData("val_and_var",
                    makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 4},
                                         Values(12), Variances(12)));

    dataset.setData("val", makeVariable<double>(Dims{Dim::X}, Shape{4}));
  }
  void expect_eq(const Dataset &a, const Dataset &b) const {
    expect_eq_impl(a, DatasetConstView(b));
    expect_eq_impl(DatasetConstView(a), b);
    expect_eq_impl(DatasetConstView(a), DatasetConstView(b));
  }
  void expect_ne(const Dataset &a, const Dataset &b) const {
    expect_ne_impl(a, DatasetConstView(b));
    expect_ne_impl(DatasetConstView(a), b);
    expect_ne_impl(DatasetConstView(a), DatasetConstView(b));
  }

  Dataset dataset;
  Variable events_variable;
};

// Baseline checks: Does dataset comparison pick up arbitrary mismatch of
// individual items? Strictly speaking many of these are just retesting the
// comparison of Variable, but it ensures that the content is actually compared
// and thus serves as a baseline for the follow-up tests.
TEST_F(Dataset_comparison_operators, single_coord) {
  auto d = make_1_coord<double>(Dim::X, {Dim::X, 3}, units::m, {1, 2, 3});
  expect_eq(d, d);
  expect_ne(d, make_empty());
  expect_ne(d, make_1_coord<float>(Dim::X, {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_coord<double>(Dim::Y, {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_coord<double>(Dim::X, {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_coord<double>(Dim::X, {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(d, make_1_coord<double>(Dim::X, {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(d, make_1_coord<double>(Dim::X, {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(Dataset_comparison_operators, single_labels) {
  auto d = make_1_labels<double>("a", {Dim::X, 3}, units::m, {1, 2, 3});
  expect_eq(d, d);
  expect_ne(d, make_empty());
  expect_ne(d, make_1_labels<float>("a", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("b", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("a", {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("a", {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(d, make_1_labels<double>("a", {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("a", {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(Dataset_comparison_operators, single_attr) {
  auto d = make_1_attr<double>("a", {Dim::X, 3}, units::m, {1, 2, 3});
  expect_eq(d, d);
  expect_ne(d, make_empty());
  expect_ne(d, make_1_attr<float>("a", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_attr<double>("b", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_attr<double>("a", {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_attr<double>("a", {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(d, make_1_attr<double>("a", {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(d, make_1_attr<double>("a", {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(Dataset_comparison_operators, single_values) {
  auto d = make_1_values<double>("a", {Dim::X, 3}, units::m, {1, 2, 3});
  expect_eq(d, d);
  expect_ne(d, make_empty());
  expect_ne(d, make_1_values<float>("a", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("b", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("a", {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("a", {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(d, make_1_values<double>("a", {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("a", {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(Dataset_comparison_operators, single_values_and_variances) {
  auto d = make_1_values_and_variances<double>("a", {Dim::X, 3}, units::m,
                                               {1, 2, 3}, {4, 5, 6});
  expect_eq(d, d);
  expect_ne(d, make_empty());
  expect_ne(d, make_1_values_and_variances<float>("a", {Dim::X, 3}, units::m,
                                                  {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>("b", {Dim::X, 3}, units::m,
                                                   {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>("a", {Dim::Y, 3}, units::m,
                                                   {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>("a", {Dim::X, 2}, units::m,
                                                   {1, 2}, {4, 5}));
  expect_ne(d, make_1_values_and_variances<double>("a", {Dim::X, 3}, units::s,
                                                   {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>("a", {Dim::X, 3}, units::m,
                                                   {1, 2, 4}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>("a", {Dim::X, 3}, units::m,
                                                   {1, 2, 3}, {4, 5, 7}));
}
// End baseline checks.

TEST_F(Dataset_comparison_operators, empty) {
  const auto empty = make_empty();
  expect_eq(empty, empty);
}

TEST_F(Dataset_comparison_operators, self) {
  expect_eq(dataset, dataset);
  const auto copy(dataset);
  expect_eq(copy, dataset);
}

TEST_F(Dataset_comparison_operators, extra_coord) {
  auto extra = dataset;
  extra.setCoord(Dim::Z, makeVariable<double>(Dims{Dim::Z}, Shape{2}));
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, extra_labels) {
  auto extra = dataset;
  extra.setCoord(Dim("extra"), makeVariable<double>(Dims{Dim::Z}, Shape{2}));
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, extra_attr) {
  auto extra = dataset;
  extra.setAttr("extra", makeVariable<double>(Dims{Dim::Z}, Shape{2}));
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, extra_data) {
  auto extra = dataset;
  extra.setData("extra", makeVariable<double>(Dims{Dim::Z}, Shape{2}));
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, extra_variance) {
  auto extra = dataset;
  extra.setData("val", makeVariable<double>(Dimensions{Dim::X, 4}, Values(4),
                                            Variances(4)));
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, different_coord_insertion_order) {
  auto a = make_empty();
  auto b = make_empty();
  a.setCoord(Dim::X, dataset.coords()[Dim::X]);
  a.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  b.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  b.setCoord(Dim::X, dataset.coords()[Dim::X]);
  expect_eq(a, b);
}

TEST_F(Dataset_comparison_operators, different_label_insertion_order) {
  auto a = make_empty();
  auto b = make_empty();
  a.setCoord(Dim("x"), dataset.coords()[Dim::X]);
  a.setCoord(Dim("y"), dataset.coords()[Dim::Y]);
  b.setCoord(Dim("y"), dataset.coords()[Dim::Y]);
  b.setCoord(Dim("x"), dataset.coords()[Dim::X]);
  expect_eq(a, b);
}

TEST_F(Dataset_comparison_operators, different_attr_insertion_order) {
  auto a = make_empty();
  auto b = make_empty();
  a.setAttr("x", dataset.coords()[Dim::X]);
  a.setAttr("y", dataset.coords()[Dim::Y]);
  b.setAttr("y", dataset.coords()[Dim::Y]);
  b.setAttr("x", dataset.coords()[Dim::X]);
  expect_eq(a, b);
}

TEST_F(Dataset_comparison_operators, different_data_insertion_order) {
  auto a = make_empty();
  auto b = make_empty();
  a.setData("x", dataset.coords()[Dim::X]);
  a.setData("y", dataset.coords()[Dim::Y]);
  b.setData("y", dataset.coords()[Dim::Y]);
  b.setData("x", dataset.coords()[Dim::X]);
  expect_eq(a, b);
}

TEST_F(Dataset_comparison_operators, with_events_dimension_data) {
  // a and b same, c different number of events values
  auto a = make_empty();
  auto data = makeVariable<event_list<double>>(Dims{}, Shape{});
  const std::string var_name = "test_var";
  data.values<event_list<double>>()[0] = {1, 2, 3};
  a.setData(var_name, data);
  auto b = make_empty();
  b.setData(var_name, data);
  expect_eq(a, b);
  data.values<event_list<double>>()[0] = {2, 3, 4};
  auto c = make_empty();
  c.setData(var_name, data);
  expect_ne(a, c);
  expect_ne(b, c);
}
