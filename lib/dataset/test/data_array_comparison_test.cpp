// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
//
// The test in this file ensure that comparison operators for DataArray are
// correct. More complex tests should build on the assumption that comparison
// operators are correct.
#include "random.h"
#include "test_macros.h"
#include <gtest/gtest.h>

#include <numeric>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::dataset;

namespace {
void expect_eq(const DataArray &a, const DataArray &b) {
  EXPECT_TRUE(a == b);
  EXPECT_TRUE(b == a);
  EXPECT_FALSE(a != b);
  EXPECT_FALSE(b != a);
  EXPECT_TRUE(equals_nan(a, b));
  EXPECT_TRUE(equals_nan(b, a));
}
void expect_ne(const DataArray &a, const DataArray &b) {
  EXPECT_TRUE(a != b);
  EXPECT_TRUE(b != a);
  EXPECT_FALSE(a == b);
  EXPECT_FALSE(b == a);
  EXPECT_FALSE(equals_nan(a, b));
  EXPECT_FALSE(equals_nan(b, a));
}
} // namespace

class DataArray_comparison_operators : public ::testing::Test {
protected:
  DataArray_comparison_operators() {
    Random rand;
    rand.seed(78847891);
    RandomBool rand_bool;
    rand_bool.seed(93481);
    da = DataArray(makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 4},
                                        Values(rand(3 * 4))),
                   {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3},
                                                  Values(rand(3)))},
                    {Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{4},
                                                  Values(rand(4)))}},
                   {{"mask", makeVariable<bool>(Dims{Dim::X}, Shape{3},
                                                Values(rand_bool(3)))}});
  }

  DataArray da;
};

namespace {
template <class T> auto make_values(const Dimensions &dims) {
  return DataArray(makeVariable<T>(Dimensions(dims)));
}

template <class T, class T2>
auto make_1_coord(const Dim dim, const Dimensions &dims,
                  const sc_units::Unit unit,
                  const std::initializer_list<T2> &data) {
  auto a = make_values<T>(dims);
  a.coords().set(dim,
                 makeVariable<T>(dims, sc_units::Unit(unit), Values(data)));
  return a;
}

template <class T, class T2>
auto make_1_labels(const std::string &name, const Dimensions &dims,
                   const sc_units::Unit unit,
                   const std::initializer_list<T2> &data) {
  auto a = make_values<T>(dims);
  a.coords().set(Dim(name),
                 makeVariable<T>(dims, sc_units::Unit(unit), Values(data)));
  return a;
}

template <class T, class T2>
auto make_1_mask(const std::string &name, const Dimensions &dims,
                 const sc_units::Unit unit,
                 const std::initializer_list<T2> &data) {
  auto a = make_values<T>(dims);
  a.masks().set(name, makeVariable<T>(Dimensions(dims), sc_units::Unit(unit),
                                      Values(data)));
  return a;
}

template <class T, class T2>
auto make_values(const std::string &name, const Dimensions &dims,
                 const sc_units::Unit unit,
                 const std::initializer_list<T2> &data) {
  DataArray da(
      makeVariable<T>(Dimensions(dims), sc_units::Unit(unit), Values(data)));
  da.setName(name);
  return da;
}

template <class T, class T2>
auto make_values_and_variances(const std::string &name, const Dimensions &dims,
                               const sc_units::Unit unit,
                               const std::initializer_list<T2> &values,
                               const std::initializer_list<T2> &variances) {
  DataArray da(makeVariable<T>(Dimensions(dims), sc_units::Unit(unit),
                               Values(values), Variances(variances)));
  da.setName(name);
  return da;
}
} // namespace

// Baseline checks: Does data-array comparison pick up arbitrary mismatch of
// individual items? Strictly speaking many of these are just retesting the
// comparison of Variable, but it ensures that the content is actually compared
// and thus serves as a baseline for the follow-up tests.
TEST_F(DataArray_comparison_operators, single_coord) {
  auto a = make_1_coord<double>(Dim::X, {Dim::X, 3}, sc_units::m,
                                {false, true, false});
  expect_eq(a, a);
  expect_ne(a, make_values<double>({Dim::X, 3}));
  expect_ne(a,
            make_1_coord<float>(Dim::X, {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(a,
            make_1_coord<double>(Dim::Y, {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(a,
            make_1_coord<double>(Dim::X, {Dim::Y, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(a, make_1_coord<double>(Dim::X, {Dim::X, 2}, sc_units::m, {1, 2}));
  expect_ne(a,
            make_1_coord<double>(Dim::X, {Dim::X, 3}, sc_units::s, {1, 2, 3}));
  expect_ne(a,
            make_1_coord<double>(Dim::X, {Dim::X, 3}, sc_units::m, {1, 2, 4}));
}

TEST_F(DataArray_comparison_operators, single_labels) {
  auto a = make_1_labels<double>("a", {Dim::X, 3}, sc_units::m, {1, 2, 3});
  expect_eq(a, a);
  expect_ne(a, make_values<double>({Dim::X, 3}));
  expect_ne(a, make_1_labels<float>("a", {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(a, make_1_labels<double>("b", {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(a, make_1_labels<double>("a", {Dim::Y, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(a, make_1_labels<double>("a", {Dim::X, 2}, sc_units::m, {1, 2}));
  expect_ne(a, make_1_labels<double>("a", {Dim::X, 3}, sc_units::s, {1, 2, 3}));
  expect_ne(a, make_1_labels<double>("a", {Dim::X, 3}, sc_units::m, {1, 2, 4}));
}

TEST_F(DataArray_comparison_operators, single_mask) {
  auto a =
      make_1_mask<bool>("a", {Dim::X, 3}, sc_units::m, {true, false, true});
  expect_eq(a, a);
  expect_ne(a, make_values<bool>({Dim::X, 3}));
  expect_ne(
      a, make_1_mask<bool>("b", {Dim::X, 3}, sc_units::m, {true, false, true}));
  expect_ne(
      a, make_1_mask<bool>("a", {Dim::Y, 3}, sc_units::m, {true, false, true}));
  expect_ne(a, make_1_mask<bool>("a", {Dim::X, 2}, sc_units::m, {true, false}));
  expect_ne(
      a, make_1_mask<bool>("a", {Dim::X, 3}, sc_units::s, {true, false, true}));
  expect_ne(a, make_1_mask<bool>("a", {Dim::X, 3}, sc_units::m,
                                 {false, false, false}));
}

TEST_F(DataArray_comparison_operators, single_values) {
  auto a = make_values<double>("a", {Dim::X, 3}, sc_units::m, {1, 2, 3});
  expect_eq(a, a);
  // Name of DataArray is ignored in comparison.
  expect_eq(a, make_values<double>("b", {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(a, make_values<double>({Dim::X, 3}));
  expect_ne(a, make_values<float>("a", {Dim::X, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(a, make_values<double>("a", {Dim::Y, 3}, sc_units::m, {1, 2, 3}));
  expect_ne(a, make_values<double>("a", {Dim::X, 2}, sc_units::m, {1, 2}));
  expect_ne(a, make_values<double>("a", {Dim::X, 3}, sc_units::s, {1, 2, 3}));
  expect_ne(a, make_values<double>("a", {Dim::X, 3}, sc_units::m, {1, 2, 4}));
}

TEST_F(DataArray_comparison_operators, single_values_and_variances) {
  auto a = make_values_and_variances<double>("a", {Dim::X, 3}, sc_units::m,
                                             {1, 2, 3}, {4, 5, 6});
  expect_eq(a, a);
  // Name of DataArray is ignored in comparison.
  expect_eq(a, make_values_and_variances<double>("b", {Dim::X, 3}, sc_units::m,
                                                 {1, 2, 3}, {4, 5, 6}));
  expect_ne(a, make_values_and_variances<float>("a", {Dim::X, 3}, sc_units::m,
                                                {1, 2, 3}, {4, 5, 6}));
  expect_ne(a, make_values_and_variances<double>("a", {Dim::Y, 3}, sc_units::m,
                                                 {1, 2, 3}, {4, 5, 6}));
  expect_ne(a, make_values_and_variances<double>("a", {Dim::X, 2}, sc_units::m,
                                                 {1, 2}, {4, 5}));
  expect_ne(a, make_values_and_variances<double>("a", {Dim::X, 3}, sc_units::s,
                                                 {1, 2, 3}, {4, 5, 6}));
  expect_ne(a, make_values_and_variances<double>("a", {Dim::X, 3}, sc_units::m,
                                                 {1, 2, 4}, {4, 5, 6}));
  expect_ne(a, make_values_and_variances<double>("a", {Dim::X, 3}, sc_units::m,
                                                 {1, 2, 3}, {4, 5, 7}));
}
// End baseline checks.

TEST_F(DataArray_comparison_operators, self) { expect_eq(da, da); }

TEST_F(DataArray_comparison_operators, copy) {
  const DataArray copy = da;
  expect_eq(copy, da);
}

TEST_F(DataArray_comparison_operators, extra_coord) {
  auto extra = da;
  extra.coords().set(Dim::Z, makeVariable<double>(Values{0.0}));
  expect_ne(extra, da);
}

TEST_F(DataArray_comparison_operators, extra_mask) {
  auto extra = da;
  extra.masks().set("extra", makeVariable<bool>(Values{false}));
  expect_ne(extra, da);
}

TEST_F(DataArray_comparison_operators, extra_variance) {
  auto extra = copy(da);
  da.data().setVariances(makeVariable<double>(da.dims()));
  expect_ne(extra, da);
}

TEST_F(DataArray_comparison_operators, different_coord_insertion_order) {
  const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 4});
  auto a = DataArray(var);
  auto b = DataArray(var);
  a.coords().set(Dim::X, da.coords()[Dim::X]);
  a.coords().set(Dim::Y, da.coords()[Dim::Y]);
  b.coords().set(Dim::Y, da.coords()[Dim::Y]);
  b.coords().set(Dim::X, da.coords()[Dim::X]);
  expect_eq(a, b);
}

TEST_F(DataArray_comparison_operators, respects_coord_alignment) {
  auto a = da;
  auto b = da;

  a.coords().set_aligned(Dim::X, false);
  expect_ne(a, b);

  b.coords().set_aligned(Dim::X, false);
  expect_eq(a, b);

  a.coords().set_aligned(Dim::X, true);
  expect_ne(a, b);
}
