// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
    EXPECT_TRUE(equals_nan(a, b));
    EXPECT_TRUE(equals_nan(b, a));
  }
  template <class A, class B>
  void expect_ne_impl(const A &a, const B &b) const {
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
    EXPECT_FALSE(equals_nan(a, b));
    EXPECT_FALSE(equals_nan(b, a));
  }

protected:
  Dataset_comparison_operators()
      : dataset(DatasetFactory{{Dim::X, 4}, {Dim::Y, 3}}.make("data")) {}
  void expect_eq(const Dataset &a, const Dataset &b) const {
    expect_eq_impl(a, b);
    expect_eq_impl(a, copy(b));
    expect_eq_impl(copy(a), b);
    expect_eq_impl(copy(a), copy(b));
  }
  void expect_ne(const Dataset &a, const Dataset &b) const {
    expect_ne_impl(a, b);
    expect_ne_impl(a, copy(b));
    expect_ne_impl(copy(a), b);
    expect_ne_impl(copy(a), copy(b));
  }

  Dataset dataset;
};

// Baseline checks: Does dataset comparison pick up arbitrary mismatch of
// individual items? Strictly speaking many of these are just retesting the
// comparison of Variable, but it ensures that the content is actually compared
// and thus serves as a baseline for the follow-up tests.
TEST_F(Dataset_comparison_operators, single_coord) {
  auto d = make_1_coord<double>(Dim::X, {Dim::X, 3}, sc_units::m, {1, 2, 3});
  expect_eq(d, d);
  expect_ne(d, Dataset{});
  expect_ne(d,
            make_1_coord<float>(Dim::X, {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(d,
            make_1_coord<double>(Dim::Y, {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(d,
            make_1_coord<double>(Dim::X, {Dim::Y, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(d, make_1_coord<double>(Dim::X, {Dim::X, 2}, sc_units::m, {1, 2}));
  expect_ne(d,
            make_1_coord<double>(Dim::X, {Dim::X, 3}, sc_units::s, {1, 2, 3}));
  expect_ne(d,
            make_1_coord<double>(Dim::X, {Dim::X, 3}, sc_units::m, {1, 2, 4}));
}

TEST_F(Dataset_comparison_operators, single_labels) {
  auto d = make_1_labels<double>("a", {Dim::X, 3}, sc_units::m, {1, 2, 3});
  expect_eq(d, d);
  expect_ne(d, Dataset{});
  expect_ne(d, make_1_labels<float>("a", {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("b", {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("a", {Dim::Y, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("a", {Dim::X, 2}, sc_units::m, {1, 2}));
  expect_ne(d, make_1_labels<double>("a", {Dim::X, 3}, sc_units::s, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("a", {Dim::X, 3}, sc_units::m, {1, 2, 4}));
}

TEST_F(Dataset_comparison_operators, single_values) {
  auto d = make_1_values<double>("a", {Dim::X, 3}, sc_units::m, {1, 2, 3});
  expect_eq(d, d);
  expect_ne(d, Dataset{});
  expect_ne(d, make_1_values<float>("a", {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("b", {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("a", {Dim::Y, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("a", {Dim::X, 2}, sc_units::m, {1, 2}));
  expect_ne(d, make_1_values<double>("a", {Dim::X, 3}, sc_units::s, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("a", {Dim::X, 3}, sc_units::m, {1, 2, 4}));
}

TEST_F(Dataset_comparison_operators, single_values_and_variances) {
  auto d = make_1_values_and_variances<double>("a", {Dim::X, 3}, sc_units::m,
                                               {1, 2, 3}, {4, 5, 6});
  expect_eq(d, d);
  expect_ne(d, Dataset{});
  expect_ne(d, make_1_values_and_variances<float>("a", {Dim::X, 3}, sc_units::m,
                                                  {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>(
                   "b", {Dim::X, 3}, sc_units::m, {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>(
                   "a", {Dim::Y, 3}, sc_units::m, {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>(
                   "a", {Dim::X, 2}, sc_units::m, {1, 2}, {4, 5}));
  expect_ne(d, make_1_values_and_variances<double>(
                   "a", {Dim::X, 3}, sc_units::s, {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>(
                   "a", {Dim::X, 3}, sc_units::m, {1, 2, 4}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>(
                   "a", {Dim::X, 3}, sc_units::m, {1, 2, 3}, {4, 5, 7}));
}
// End baseline checks.

TEST_F(Dataset_comparison_operators, empty) {
  const auto empty = Dataset{};
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

TEST_F(Dataset_comparison_operators, extra_data) {
  auto extra = dataset;
  extra.setData("extra", dataset["data"]);
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, extra_variance) {
  auto extra = copy(dataset);
  extra["data"].data().setVariances(makeVariable<double>(extra["data"].dims()));
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, different_coord_insertion_order) {
  Dataset a({{"a", dataset["data"].data()}});
  Dataset b({{"a", dataset["data"].data()}});
  a.setCoord(Dim::X, dataset.coords()[Dim::X]);
  a.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  b.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  b.setCoord(Dim::X, dataset.coords()[Dim::X]);
  expect_eq(a, b);
}

TEST_F(Dataset_comparison_operators, different_data_insertion_order) {
  auto xy1 = dataset.coords()[Dim::X] * dataset.coords()[Dim::Y];
  auto xy2 = dataset.coords()[Dim::X] + dataset.coords()[Dim::Y];
  Dataset a({{"x", xy1}, {"y", xy2}});
  Dataset b({{"y", xy2}, {"x", xy1}});
  expect_eq(a, b);
}
