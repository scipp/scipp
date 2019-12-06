// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(Variable, construct_default) {
  ASSERT_NO_THROW(Variable{});
  Variable var;
  ASSERT_FALSE(var);
}

TEST(Variable, construct) {
  ASSERT_NO_THROW(createVariable<double>(Dims{Dim::X}, Shape{2}));
  ASSERT_NO_THROW(createVariable<double>(Dims{Dim::X}, Shape{2}, Values(2)));
  const auto a = createVariable<double>(Dims{Dim::X}, Shape{2});
  const auto &data = a.values<double>();
  EXPECT_EQ(data.size(), 2);
}

TEST(Variable, construct_fail) {
  ASSERT_ANY_THROW(createVariable<double>(Dims{}, Shape{}, Values(2)));
  ASSERT_ANY_THROW(createVariable<double>(Dims{Dim::X}, Shape{1}, Values(2)));
  ASSERT_ANY_THROW(createVariable<double>(Dims{Dim::X}, Shape{3}, Values(2)));
}

TEST(Variable, move) {
  auto var = createVariable<double>(Dims{Dim::X}, Shape{2});
  Variable moved(std::move(var));
  EXPECT_FALSE(var);
  EXPECT_NE(moved, var);
}

TEST(Variable, makeVariable_custom_type) {
  auto doubles = makeVariable<double>({});
  auto floats = makeVariable<float>({});

  ASSERT_NO_THROW(doubles.values<double>());
  ASSERT_NO_THROW(floats.values<float>());

  ASSERT_ANY_THROW(doubles.values<float>());
  ASSERT_ANY_THROW(floats.values<double>());

  ASSERT_TRUE((std::is_same<decltype(doubles.values<double>())::element_type,
                            double>::value));
  ASSERT_TRUE((std::is_same<decltype(floats.values<float>())::element_type,
                            float>::value));
}

TEST(Variable, makeVariable_custom_type_initializer_list) {
  auto doubles = createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  auto ints = createVariable<int32_t>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});

  // Passed ints but uses default type based on tag.
  ASSERT_NO_THROW(doubles.values<double>());
  // Passed doubles but explicit type overrides.
  ASSERT_NO_THROW(ints.values<int32_t>());
}

TEST(Variable, dtype) {
  auto doubles = makeVariable<double>({});
  auto floats = makeVariable<float>({});
  EXPECT_EQ(doubles.dtype(), dtype<double>);
  EXPECT_NE(doubles.dtype(), dtype<float>);
  EXPECT_NE(floats.dtype(), dtype<double>);
  EXPECT_EQ(floats.dtype(), dtype<float>);
  EXPECT_EQ(doubles.dtype(), doubles.dtype());
  EXPECT_EQ(floats.dtype(), floats.dtype());
  EXPECT_NE(doubles.dtype(), floats.dtype());
}

TEST(Variable, span_references_Variable) {
  auto a = createVariable<double>(Dims{Dim::X}, Shape{2});
  auto observer = a.values<double>();
  // This line does not compile, const-correctness works:
  // observer[0] = 1.0;

  auto span = a.values<double>();

  EXPECT_EQ(span.size(), 2);
  span[0] = 1.0;
  EXPECT_EQ(observer[0], 1.0);
}

class Variable_comparison_operators : public ::testing::Test {
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
  void expect_eq(const Variable &a, const Variable &b) const {
    expect_eq_impl(a, VariableConstProxy(b));
    expect_eq_impl(VariableConstProxy(a), b);
    expect_eq_impl(VariableConstProxy(a), VariableConstProxy(b));
  }
  void expect_ne(const Variable &a, const Variable &b) const {
    expect_ne_impl(a, VariableConstProxy(b));
    expect_ne_impl(VariableConstProxy(a), b);
    expect_ne_impl(VariableConstProxy(a), VariableConstProxy(b));
  }
};

TEST_F(Variable_comparison_operators, values_0d) {
  const auto base = makeVariable<double>(1.1);
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>(1.1));
  expect_ne(base, makeVariable<double>(1.2));
}

TEST_F(Variable_comparison_operators, values_1d) {
  const auto base =
      createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  expect_eq(base, base);
  expect_eq(base,
            createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2}));
  expect_ne(base,
            createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.3}));
}

TEST_F(Variable_comparison_operators, values_2d) {
  const auto base = createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                           Values{1.1, 2.2});
  expect_eq(base, base);
  expect_eq(base, createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                         Values{1.1, 2.2}));
  expect_ne(base, createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                         Values{1.1, 2.3}));
}

TEST_F(Variable_comparison_operators, variances_0d) {
  const auto base = makeVariable<double>(1.1, 0.1);
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>(1.1, 0.1));
  expect_ne(base, makeVariable<double>(1.1));
  expect_ne(base, makeVariable<double>(1.1, 0.2));
}

TEST_F(Variable_comparison_operators, variances_1d) {
  const auto base = createVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{1.1, 2.2}, Variances{0.1, 0.2});
  expect_eq(base, base);
  expect_eq(base,
            createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2},
                                   Variances{0.1, 0.2}));
  expect_ne(base,
            createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2}));
  expect_ne(base,
            createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2},
                                   Variances{0.1, 0.3}));
}

TEST_F(Variable_comparison_operators, variances_2d) {
  const auto base = createVariable<double>(
      Dims{Dim::X, Dim::Y}, Shape{2, 1}, Values{1.1, 2.2}, Variances{0.1, 0.2});
  expect_eq(base, base);
  expect_eq(base,
            createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                   Values{1.1, 2.2}, Variances{0.1, 0.2}));
  expect_ne(base, createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                         Values{1.1, 2.2}));
  expect_ne(base,
            createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                   Values{1.1, 2.2}, Variances{0.1, 0.3}));
}

TEST_F(Variable_comparison_operators, dimension_mismatch) {
  expect_ne(makeVariable<double>(1.1),
            createVariable<double>(Dims{Dim::X}, Shape{1}, Values{1.1}));
  expect_ne(createVariable<double>(Dims{Dim::X}, Shape{1}, Values{1.1}),
            createVariable<double>(Dims{Dim::Y}, Shape{1}, Values{1.1}));
}

TEST_F(Variable_comparison_operators, dimension_transpose) {
  expect_ne(
      createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{1, 1}, Values{1.1}),
      createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{1, 1}, Values{1.1}));
}

TEST_F(Variable_comparison_operators, dimension_length) {
  expect_ne(createVariable<double>(Dims{Dim::X}, Shape{1}),
            createVariable<double>(Dims{Dim::X}, Shape{2}));
}

TEST_F(Variable_comparison_operators, unit) {
  const auto m = createVariable<double>(Dims{Dim::X}, Shape{1},
                                        units::Unit(units::m), Values{1.1});
  const auto s = createVariable<double>(Dims{Dim::X}, Shape{1},
                                        units::Unit(units::s), Values{1.1});
  expect_eq(m, m);
  expect_ne(m, s);
}

TEST_F(Variable_comparison_operators, dtype) {
  const auto base = makeVariable<double>(1.0);
  expect_ne(base, makeVariable<float>(1.0));
}

TEST_F(Variable_comparison_operators, dense_sparse) {
  auto dense = makeVariable<double>({Dim::Y, Dim::X}, {2, 0});
  auto sparse = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  expect_ne(dense, sparse);
}

TEST_F(Variable_comparison_operators, sparse) {
  auto a = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a_ = a.sparseValues<double>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};
  auto b = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto b_ = b.sparseValues<double>();
  b_[0] = {1, 2, 3};
  b_[1] = {1, 2};
  auto c = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto c_ = c.sparseValues<double>();
  c_[0] = {1, 3};
  c_[1] = {};

  expect_eq(a, a);
  expect_eq(a, b);
  expect_ne(a, c);
}

auto make_sparse_var_2d_with_variances() {
  auto var = makeVariableWithVariances<double>(
      {{Dim::Y, Dim::X}, {2, Dimensions::Sparse}});
  auto vals = var.sparseValues<double>();
  vals[0] = {1, 2, 3};
  vals[1] = {1, 2};
  auto vars = var.sparseVariances<double>();
  vars[0] = {4, 5, 6};
  vars[1] = {4, 5};
  return var;
}

TEST_F(Variable_comparison_operators, sparse_variances) {
  const auto a = make_sparse_var_2d_with_variances();
  const auto b = make_sparse_var_2d_with_variances();
  auto c = makeVariableWithVariances<double>(
      {{Dim::Y, Dim::X}, {2, Dimensions::Sparse}});
  auto c_vals = c.sparseValues<double>();
  c_vals[0] = {1, 2, 3};
  c_vals[1] = {1, 2};
  auto c_vars = c.sparseVariances<double>();
  c_vars[0] = {1, 3};
  c_vars[1] = {};

  auto a_no_vars =
      makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a_no_vars_ = a_no_vars.sparseValues<double>();
  a_no_vars_[0] = {1, 2, 3};
  a_no_vars_[1] = {1, 2};

  expect_eq(a, a);
  expect_eq(a, b);

  expect_ne(a, c);
  expect_ne(a, a_no_vars);
}

TEST(VariableTest, copy_and_move) {
  const auto reference = createVariable<double>(
      Dims{Dim::X, Dim::Y}, Shape{2, 1}, units::Unit(units::m),
      Values{1.1, 2.2}, Variances{0.1, 0.2});
  const auto var = createVariable<double>(
      Dims{Dim::X, Dim::Y}, Shape{2, 1}, units::Unit(units::m),
      Values{1.1, 2.2}, Variances{0.1, 0.2});

  const auto copy(var);
  EXPECT_EQ(copy, reference);

  const Variable copy_via_slice{VariableConstProxy(var)};
  EXPECT_EQ(copy_via_slice, reference);

  const auto moved(std::move(var));
  EXPECT_EQ(moved, reference);
}

TEST(Variable, assign_slice) {
  const auto parent = createVariable<double>(
      Dims{Dim::X, Dim::Y, Dim::Z}, Shape{4, 2, 3},
      Values{1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
             13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},
      Variances{25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
                37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48});
  const auto empty = makeVariableWithVariances<double>(
      {{Dim::X, 4}, {Dim::Y, 2}, {Dim::Z, 3}});

  auto d(empty);
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1, 2, 3})
    d.slice({Dim::X, index}).assign(parent.slice({Dim::X, index}));
  EXPECT_EQ(parent, d);

  d = empty;
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1})
    d.slice({Dim::Y, index}).assign(parent.slice({Dim::Y, index}));
  EXPECT_EQ(parent, d);

  d = empty;
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1, 2})
    d.slice({Dim::Z, index}).assign(parent.slice({Dim::Z, index}));
  EXPECT_EQ(parent, d);
}

TEST(Variable, assign_slice_unit_checks) {
  const auto parent =
      createVariable<double>(Dims(), Shape(), units::Unit(units::m), Values{1});
  auto dimensionless = createVariable<double>(Dims{Dim::X}, Shape{4});
  auto m =
      createVariable<double>(Dims{Dim::X}, Shape{4}, units::Unit(units::m));

  EXPECT_THROW(dimensionless.slice({Dim::X, 1}).assign(parent),
               except::UnitError);
  EXPECT_NO_THROW(m.slice({Dim::X, 1}).assign(parent));
}

TEST(Variable, assign_slice_variance_checks) {
  const auto parent_vals = makeVariable<double>(1.0);
  const auto parent_vals_vars = makeVariable<double>(1.0, 2.0);
  auto vals = createVariable<double>(Dims{Dim::X}, Shape{4});
  auto vals_vars = makeVariableWithVariances<double>({Dim::X, 4});

  EXPECT_NO_THROW(vals.slice({Dim::X, 1}).assign(parent_vals));
  EXPECT_NO_THROW(vals_vars.slice({Dim::X, 1}).assign(parent_vals_vars));
  EXPECT_THROW(vals.slice({Dim::X, 1}).assign(parent_vals_vars),
               except::VariancesError);
  EXPECT_THROW(vals_vars.slice({Dim::X, 1}).assign(parent_vals),
               except::VariancesError);
}

class VariableTest_3d : public ::testing::Test {
protected:
  const Variable parent{createVariable<double>(
      Dims{Dim::X, Dim::Y, Dim::Z}, Shape{4, 2, 3}, units::Unit(units::m),
      Values{1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
             13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},
      Variances{25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
                37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48})};
  const std::vector<double> vals_x0{1, 2, 3, 4, 5, 6};
  const std::vector<double> vals_x1{7, 8, 9, 10, 11, 12};
  const std::vector<double> vals_x2{13, 14, 15, 16, 17, 18};
  const std::vector<double> vals_x3{19, 20, 21, 22, 23, 24};
  const std::vector<double> vars_x0{25, 26, 27, 28, 29, 30};
  const std::vector<double> vars_x1{31, 32, 33, 34, 35, 36};
  const std::vector<double> vars_x2{37, 38, 39, 40, 41, 42};
  const std::vector<double> vars_x3{43, 44, 45, 46, 47, 48};

  const std::vector<double> vals_x02{
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  };
  const std::vector<double> vals_x13{7,  8,  9,  10, 11, 12,
                                     13, 14, 15, 16, 17, 18};
  const std::vector<double> vals_x24{13, 14, 15, 16, 17, 18,
                                     19, 20, 21, 22, 23, 24};
  const std::vector<double> vars_x02{25, 26, 27, 28, 29, 30,
                                     31, 32, 33, 34, 35, 36};
  const std::vector<double> vars_x13{31, 32, 33, 34, 35, 36,
                                     37, 38, 39, 40, 41, 42};
  const std::vector<double> vars_x24{37, 38, 39, 40, 41, 42,
                                     43, 44, 45, 46, 47, 48};

  const std::vector<double> vals_y0{1, 2, 3, 7, 8, 9, 13, 14, 15, 19, 20, 21};
  const std::vector<double> vals_y1{4,  5,  6,  10, 11, 12,
                                    16, 17, 18, 22, 23, 24};
  const std::vector<double> vars_y0{25, 26, 27, 31, 32, 33,
                                    37, 38, 39, 43, 44, 45};
  const std::vector<double> vars_y1{28, 29, 30, 34, 35, 36,
                                    40, 41, 42, 46, 47, 48};

  const std::vector<double> vals_z0{1, 4, 7, 10, 13, 16, 19, 22};
  const std::vector<double> vals_z1{2, 5, 8, 11, 14, 17, 20, 23};
  const std::vector<double> vals_z2{3, 6, 9, 12, 15, 18, 21, 24};
  const std::vector<double> vars_z0{25, 28, 31, 34, 37, 40, 43, 46};
  const std::vector<double> vars_z1{26, 29, 32, 35, 38, 41, 44, 47};
  const std::vector<double> vars_z2{27, 30, 33, 36, 39, 42, 45, 48};

  const std::vector<double> vals_z02{1,  2,  4,  5,  7,  8,  10, 11,
                                     13, 14, 16, 17, 19, 20, 22, 23};
  const std::vector<double> vals_z13{2,  3,  5,  6,  8,  9,  11, 12,
                                     14, 15, 17, 18, 20, 21, 23, 24};
  const std::vector<double> vars_z02{25, 26, 28, 29, 31, 32, 34, 35,
                                     37, 38, 40, 41, 43, 44, 46, 47};
  const std::vector<double> vars_z13{26, 27, 29, 30, 32, 33, 35, 36,
                                     38, 39, 41, 42, 44, 45, 47, 48};
};

TEST_F(VariableTest_3d, slice_single) {
  Dimensions dims_no_x{{Dim::Y, 2}, {Dim::Z, 3}};
  EXPECT_EQ(parent.slice({Dim::X, 0}),
            createVariable<double>(Dimensions(dims_no_x), units::Unit(units::m),
                                   Values(vals_x0.begin(), vals_x0.end()),
                                   Variances(vars_x0.begin(), vars_x0.end())));
  EXPECT_EQ(parent.slice({Dim::X, 1}),
            createVariable<double>(Dimensions(dims_no_x), units::Unit(units::m),
                                   Values(vals_x1.begin(), vals_x1.end()),
                                   Variances(vars_x1.begin(), vars_x1.end())));
  EXPECT_EQ(parent.slice({Dim::X, 2}),
            createVariable<double>(Dimensions(dims_no_x), units::Unit(units::m),
                                   Values(vals_x2.begin(), vals_x2.end()),
                                   Variances(vars_x2.begin(), vars_x2.end())));
  EXPECT_EQ(parent.slice({Dim::X, 3}),
            createVariable<double>(Dimensions(dims_no_x), units::Unit(units::m),
                                   Values(vals_x3.begin(), vals_x3.end()),
                                   Variances(vars_x3.begin(), vars_x3.end())));

  Dimensions dims_no_y{{Dim::X, 4}, {Dim::Z, 3}};
  EXPECT_EQ(parent.slice({Dim::Y, 0}),
            createVariable<double>(Dimensions(dims_no_y), units::Unit(units::m),
                                   Values(vals_y0.begin(), vals_y0.end()),
                                   Variances(vars_y0.begin(), vars_y0.end())));
  EXPECT_EQ(parent.slice({Dim::Y, 1}),
            createVariable<double>(Dimensions(dims_no_y), units::Unit(units::m),
                                   Values(vals_y1.begin(), vals_y1.end()),
                                   Variances(vars_y1.begin(), vars_y1.end())));

  Dimensions dims_no_z{{Dim::X, 4}, {Dim::Y, 2}};
  EXPECT_EQ(parent.slice({Dim::Z, 0}),
            createVariable<double>(Dimensions(dims_no_z), units::Unit(units::m),
                                   Values(vals_z0.begin(), vals_z0.end()),
                                   Variances(vars_z0.begin(), vars_z0.end())));
  EXPECT_EQ(parent.slice({Dim::Z, 1}),
            createVariable<double>(Dimensions(dims_no_z), units::Unit(units::m),
                                   Values(vals_z1.begin(), vals_z1.end()),
                                   Variances(vars_z1.begin(), vars_z1.end())));
  EXPECT_EQ(parent.slice({Dim::Z, 2}),
            createVariable<double>(Dimensions(dims_no_z), units::Unit(units::m),
                                   Values(vals_z2.begin(), vals_z2.end()),
                                   Variances(vars_z2.begin(), vars_z2.end())));
}

TEST_F(VariableTest_3d, slice_range) {
  // Length 1 slice
  Dimensions dims_x1{{Dim::X, 1}, {Dim::Y, 2}, {Dim::Z, 3}};
  EXPECT_EQ(parent.slice({Dim::X, 0, 1}),
            createVariable<double>(Dimensions(dims_x1), units::Unit(units::m),
                                   Values(vals_x0.begin(), vals_x0.end()),
                                   Variances(vars_x0.begin(), vars_x0.end())));
  EXPECT_EQ(parent.slice({Dim::X, 1, 2}),
            createVariable<double>(Dimensions(dims_x1), units::Unit(units::m),
                                   Values(vals_x1.begin(), vals_x1.end()),
                                   Variances(vars_x1.begin(), vars_x1.end())));
  EXPECT_EQ(parent.slice({Dim::X, 2, 3}),
            createVariable<double>(Dimensions(dims_x1), units::Unit(units::m),
                                   Values(vals_x2.begin(), vals_x2.end()),
                                   Variances(vars_x2.begin(), vars_x2.end())));
  EXPECT_EQ(parent.slice({Dim::X, 3, 4}),
            createVariable<double>(Dimensions(dims_x1), units::Unit(units::m),
                                   Values(vals_x3.begin(), vals_x3.end()),
                                   Variances(vars_x3.begin(), vars_x3.end())));

  Dimensions dims_y1{{Dim::X, 4}, {Dim::Y, 1}, {Dim::Z, 3}};
  EXPECT_EQ(parent.slice({Dim::Y, 0, 1}),
            createVariable<double>(Dimensions(dims_y1), units::Unit(units::m),
                                   Values(vals_y0.begin(), vals_y0.end()),
                                   Variances(vars_y0.begin(), vars_y0.end())));
  EXPECT_EQ(parent.slice({Dim::Y, 1, 2}),
            createVariable<double>(Dimensions(dims_y1), units::Unit(units::m),
                                   Values(vals_y1.begin(), vals_y1.end()),
                                   Variances(vars_y1.begin(), vars_y1.end())));

  Dimensions dims_z1{{Dim::X, 4}, {Dim::Y, 2}, {Dim::Z, 1}};
  EXPECT_EQ(parent.slice({Dim::Z, 0, 1}),
            createVariable<double>(Dimensions(dims_z1), units::Unit(units::m),
                                   Values(vals_z0.begin(), vals_z0.end()),
                                   Variances(vars_z0.begin(), vars_z0.end())));
  EXPECT_EQ(parent.slice({Dim::Z, 1, 2}),
            createVariable<double>(Dimensions(dims_z1), units::Unit(units::m),
                                   Values(vals_z1.begin(), vals_z1.end()),
                                   Variances(vars_z1.begin(), vars_z1.end())));
  EXPECT_EQ(parent.slice({Dim::Z, 2, 3}),
            createVariable<double>(Dimensions(dims_z1), units::Unit(units::m),
                                   Values(vals_z2.begin(), vals_z2.end()),
                                   Variances(vars_z2.begin(), vars_z2.end())));

  // Length 2 slice
  Dimensions dims_x2{{Dim::X, 2}, {Dim::Y, 2}, {Dim::Z, 3}};
  EXPECT_EQ(
      parent.slice({Dim::X, 0, 2}),
      createVariable<double>(Dimensions(dims_x2), units::Unit(units::m),
                             Values(vals_x02.begin(), vals_x02.end()),
                             Variances(vars_x02.begin(), vars_x02.end())));
  EXPECT_EQ(
      parent.slice({Dim::X, 1, 3}),
      createVariable<double>(Dimensions(dims_x2), units::Unit(units::m),
                             Values(vals_x13.begin(), vals_x13.end()),
                             Variances(vars_x13.begin(), vars_x13.end())));
  EXPECT_EQ(
      parent.slice({Dim::X, 2, 4}),
      createVariable<double>(Dimensions(dims_x2), units::Unit(units::m),
                             Values(vals_x24.begin(), vals_x24.end()),
                             Variances(vars_x24.begin(), vars_x24.end())));

  EXPECT_EQ(parent.slice({Dim::Y, 0, 2}), parent);

  Dimensions dims_z2{{Dim::X, 4}, {Dim::Y, 2}, {Dim::Z, 2}};
  EXPECT_EQ(
      parent.slice({Dim::Z, 0, 2}),
      createVariable<double>(Dimensions(dims_z2), units::Unit(units::m),
                             Values(vals_z02.begin(), vals_z02.end()),
                             Variances(vars_z02.begin(), vars_z02.end())));
  EXPECT_EQ(
      parent.slice({Dim::Z, 1, 3}),
      createVariable<double>(Dimensions(dims_z2), units::Unit(units::m),
                             Values(vals_z13.begin(), vals_z13.end()),
                             Variances(vars_z13.begin(), vars_z13.end())));
}

TEST(Variable, broadcast) {
  auto reference =
      createVariable<double>(Dims{Dim::Z, Dim::Y, Dim::X}, Shape{3, 2, 2},
                             Values{1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4},
                             Variances{5, 6, 7, 8, 5, 6, 7, 8, 5, 6, 7, 8});
  auto var = createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                    Values{1, 2, 3, 4}, Variances{5, 6, 7, 8});

  // No change if dimensions exist.
  EXPECT_EQ(broadcast(var, {Dim::X, 2}), var);
  EXPECT_EQ(broadcast(var, {Dim::Y, 2}), var);
  EXPECT_EQ(broadcast(var, {{Dim::Y, 2}, {Dim::X, 2}}), var);

  // No transpose done, should this fail? Failing is not really necessary since
  // we have labeled dimensions.
  EXPECT_EQ(broadcast(var, {{Dim::X, 2}, {Dim::Y, 2}}), var);

  EXPECT_EQ(broadcast(var, {Dim::Z, 3}), reference);
}

TEST(Variable, broadcast_fail) {
  auto var = createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                    Values{1, 2, 3, 4});
  EXPECT_THROW_MSG(broadcast(var, {Dim::X, 3}), except::DimensionLengthError,
                   "Expected dimension to be in {{Dim.Y, 2}, {Dim.X, 2}}, "
                   "got Dim.X with mismatching length 3.");
}

TEST(VariableProxy, full_const_view) {
  const auto var = makeVariableWithVariances<double>({Dim::X, 3});
  VariableConstProxy view(var);
  EXPECT_EQ(&var.values<double>()[0], &view.values<double>()[0]);
  EXPECT_EQ(&var.variances<double>()[0], &view.variances<double>()[0]);
}

TEST(VariableProxy, full_mutable_view) {
  auto var = makeVariableWithVariances<double>({Dim::X, 3});
  VariableProxy view(var);
  EXPECT_EQ(&var.values<double>()[0], &view.values<double>()[0]);
  EXPECT_EQ(&var.variances<double>()[0], &view.variances<double>()[0]);
}

TEST(VariableProxy, strides) {
  auto var = createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
  EXPECT_EQ(var.slice({Dim::X, 0}).strides(), (std::vector<scipp::index>{3}));
  EXPECT_EQ(var.slice({Dim::X, 1}).strides(), (std::vector<scipp::index>{3}));
  EXPECT_EQ(var.slice({Dim::Y, 0}).strides(), (std::vector<scipp::index>{1}));
  EXPECT_EQ(var.slice({Dim::Y, 1}).strides(), (std::vector<scipp::index>{1}));
  EXPECT_EQ(var.slice({Dim::X, 0, 1}).strides(),
            (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var.slice({Dim::X, 1, 2}).strides(),
            (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var.slice({Dim::Y, 0, 1}).strides(),
            (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var.slice({Dim::Y, 1, 2}).strides(),
            (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var.slice({Dim::X, 0, 2}).strides(),
            (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var.slice({Dim::X, 1, 3}).strides(),
            (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var.slice({Dim::Y, 0, 2}).strides(),
            (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var.slice({Dim::Y, 1, 3}).strides(),
            (std::vector<scipp::index>{3, 1}));

  EXPECT_EQ(var.slice({Dim::X, 0, 1}).slice({Dim::Y, 0, 1}).strides(),
            (std::vector<scipp::index>{3, 1}));

  auto var3D =
      createVariable<double>(Dims{Dim::Z, Dim::Y, Dim::X}, Shape{4, 3, 2});
  EXPECT_EQ(var3D.slice({Dim::X, 0, 1}).slice({Dim::Z, 0, 1}).strides(),
            (std::vector<scipp::index>{6, 2, 1}));
}

TEST(VariableProxy, values_and_variances) {
  const auto var = createVariable<double>(Dims{Dim::X}, Shape{3},
                                          Values{1, 2, 3}, Variances{4, 5, 6});
  const auto proxy = var.slice({Dim::X, 1, 2});
  EXPECT_EQ(proxy.values<double>().size(), 1);
  EXPECT_EQ(proxy.values<double>()[0], 2.0);
  EXPECT_EQ(proxy.variances<double>().size(), 1);
  EXPECT_EQ(proxy.variances<double>()[0], 5.0);
}

TEST(VariableProxy, slicing_does_not_transpose) {
  auto var = createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 3});
  Dimensions expected{{Dim::X, 1}, {Dim::Y, 1}};
  EXPECT_EQ(var.slice({Dim::X, 1, 2}).slice({Dim::Y, 1, 2}).dims(), expected);
  EXPECT_EQ(var.slice({Dim::Y, 1, 2}).slice({Dim::X, 1, 2}).dims(), expected);
}

TEST(VariableProxy, variable_copy_from_slice) {
  const auto source = createVariable<double>(
      Dims{Dim::Y, Dim::X}, Shape{3, 3}, units::Unit(units::m),
      Values{11, 12, 13, 21, 22, 23, 31, 32, 33},
      Variances{44, 45, 46, 54, 55, 56, 64, 65, 66});

  const Dimensions dims({{Dim::Y, 2}, {Dim::X, 2}});
  EXPECT_EQ(source.slice({Dim::X, 0, 2}).slice({Dim::Y, 0, 2}),
            createVariable<double>(Dimensions(dims), units::Unit(units::m),
                                   Values{11, 12, 21, 22},
                                   Variances{44, 45, 54, 55}));

  EXPECT_EQ(source.slice({Dim::X, 1, 3}).slice({Dim::Y, 0, 2}),
            createVariable<double>(Dimensions(dims), units::Unit(units::m),
                                   Values{12, 13, 22, 23},
                                   Variances{45, 46, 55, 56}));

  EXPECT_EQ(source.slice({Dim::X, 0, 2}).slice({Dim::Y, 1, 3}),
            createVariable<double>(Dimensions(dims), units::Unit(units::m),
                                   Values{21, 22, 31, 32},
                                   Variances{54, 55, 64, 65}));

  EXPECT_EQ(source.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3}),
            createVariable<double>(Dimensions(dims), units::Unit(units::m),
                                   Values{22, 23, 32, 33},
                                   Variances{55, 56, 65, 66}));
}

TEST(VariableProxy, variable_assign_from_slice) {
  const Dimensions dims({{Dim::Y, 2}, {Dim::X, 2}});
  // Unit is dimensionless and no variance
  auto target = createVariable<double>(Dimensions(dims), Values{1, 2, 3, 4});
  const auto source = createVariable<double>(
      Dims{Dim::Y, Dim::X}, Shape{3, 3}, units::Unit(units::m),
      Values{11, 12, 13, 21, 22, 23, 31, 32, 33},
      Variances{44, 45, 46, 54, 55, 56, 64, 65, 66});

  target = Variable(source.slice({Dim::X, 0, 2}).slice({Dim::Y, 0, 2}));
  EXPECT_EQ(target, createVariable<double>(
                        Dimensions(dims), units::Unit(units::m),
                        Values{11, 12, 21, 22}, Variances{44, 45, 54, 55}));

  target = Variable(source.slice({Dim::X, 1, 3}).slice({Dim::Y, 0, 2}));
  EXPECT_EQ(target, createVariable<double>(
                        Dimensions(dims), units::Unit(units::m),
                        Values{12, 13, 22, 23}, Variances{45, 46, 55, 56}));

  target = Variable(source.slice({Dim::X, 0, 2}).slice({Dim::Y, 1, 3}));
  EXPECT_EQ(target, createVariable<double>(
                        Dimensions(dims), units::Unit(units::m),
                        Values{21, 22, 31, 32}, Variances{54, 55, 64, 65}));

  target = Variable(source.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3}));
  EXPECT_EQ(target, createVariable<double>(
                        Dimensions(dims), units::Unit(units::m),
                        Values{22, 23, 32, 33}, Variances{55, 56, 65, 66}));
}

TEST(VariableProxy, variable_assign_from_slice_clears_variances) {
  const Dimensions dims({{Dim::Y, 2}, {Dim::X, 2}});
  auto target = createVariable<double>(Dimensions(dims), Values{1, 2, 3, 4},
                                       Variances{5, 6, 7, 8});
  const auto source = createVariable<double>(
      Dims{Dim::Y, Dim::X}, Shape{3, 3}, units::Unit(units::m),
      Values{11, 12, 13, 21, 22, 23, 31, 32, 33});

  target = Variable(source.slice({Dim::X, 0, 2}).slice({Dim::Y, 0, 2}));
  EXPECT_EQ(target,
            createVariable<double>(Dimensions(dims), units::Unit(units::m),
                                   Values{11, 12, 21, 22}));
}

TEST(VariableProxy, variable_self_assign_via_slice) {
  auto target =
      createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3},
                             Values{11, 12, 13, 21, 22, 23, 31, 32, 33},
                             Variances{44, 45, 46, 54, 55, 56, 64, 65, 66});

  target = Variable(target.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3}));
  // Note: This test does not actually fail if self-assignment is broken. Had to
  // run address sanitizer to see that it is reading from free'ed memory.
  EXPECT_EQ(target, createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                           Values{22, 23, 32, 33},
                                           Variances{55, 56, 65, 66}));
}

TEST(VariableProxy, slice_assign_from_variable_unit_fail) {
  const auto source =
      createVariable<double>(Dims{Dim::X}, Shape{1}, units::Unit(units::m));
  auto target = createVariable<double>(Dims{Dim::X}, Shape{2});
  EXPECT_THROW(target.slice({Dim::X, 1, 2}).assign(source), except::UnitError);
  target =
      createVariable<double>(Dims{Dim::X}, Shape{2}, units::Unit(units::m));
  EXPECT_NO_THROW(target.slice({Dim::X, 1, 2}).assign(source));
}

TEST(VariableProxy, slice_assign_from_variable_full_slice_can_change_unit) {
  const auto source =
      createVariable<double>(Dims{Dim::X}, Shape{2}, units::Unit(units::m));
  auto target = createVariable<double>(Dims{Dim::X}, Shape{2});
  target.slice({Dim::X, 0, 2}).assign(source);
  EXPECT_NO_THROW(target.slice({Dim::X, 0, 2}).assign(source));
}

TEST(VariableProxy, slice_assign_from_variable_variance_fail) {
  const auto vals = createVariable<double>(Dims{Dim::X}, Shape{1});
  const auto vals_vars = makeVariableWithVariances<double>({Dim::X, 1});

  auto target = createVariable<double>(Dims{Dim::X}, Shape{2});
  EXPECT_THROW(target.slice({Dim::X, 1, 2}).assign(vals_vars),
               except::VariancesError);
  EXPECT_NO_THROW(target.slice({Dim::X, 1, 2}).assign(vals));

  target = makeVariableWithVariances<double>({Dim::X, 2});
  EXPECT_THROW(target.slice({Dim::X, 1, 2}).assign(vals),
               except::VariancesError);
  EXPECT_NO_THROW(target.slice({Dim::X, 1, 2}).assign(vals_vars));
}

TEST(VariableProxy, slice_assign_from_variable) {
  const auto source =
      createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                             Values{11, 12, 21, 22}, Variances{33, 34, 43, 44});

  // We might want to mimick Python's __setitem__, but operator= would (and
  // should!?) assign the view contents, not the data.
  const Dimensions dims({{Dim::Y, 3}, {Dim::X, 3}});
  auto target = makeVariableWithVariances<double>(dims);
  target.slice({Dim::X, 0, 2}).slice({Dim::Y, 0, 2}).assign(source);
  EXPECT_EQ(target, createVariable<double>(
                        Dimensions(dims), Values{11, 12, 0, 21, 22, 0, 0, 0, 0},
                        Variances{33, 34, 0, 43, 44, 0, 0, 0, 0}));

  target = makeVariableWithVariances<double>(dims);
  target.slice({Dim::X, 1, 3}).slice({Dim::Y, 0, 2}).assign(source);
  EXPECT_EQ(target, createVariable<double>(
                        Dimensions(dims), Values{0, 11, 12, 0, 21, 22, 0, 0, 0},
                        Variances{0, 33, 34, 0, 43, 44, 0, 0, 0}));

  target = makeVariableWithVariances<double>(dims);
  target.slice({Dim::X, 0, 2}).slice({Dim::Y, 1, 3}).assign(source);
  EXPECT_EQ(target, createVariable<double>(
                        Dimensions(dims), Values{0, 0, 0, 11, 12, 0, 21, 22, 0},
                        Variances{0, 0, 0, 33, 34, 0, 43, 44, 0}));

  target = makeVariableWithVariances<double>(dims);
  target.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3}).assign(source);
  EXPECT_EQ(target, createVariable<double>(
                        Dimensions(dims), Values{0, 0, 0, 0, 11, 12, 0, 21, 22},
                        Variances{0, 0, 0, 0, 33, 34, 0, 43, 44}));
}

TEST(VariableTest, reshape) {
  const auto var =
      createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                             units::Unit(units::m), Values{1, 2, 3, 4, 5, 6});

  ASSERT_EQ(var.reshape({Dim::Row, 6}),
            createVariable<double>(Dims{Dim::Row}, Shape{6},
                                   units::Unit(units::m),
                                   Values{1, 2, 3, 4, 5, 6}));
  ASSERT_EQ(var.reshape({{Dim::Row, 3}, {Dim::Z, 2}}),
            createVariable<double>(Dims{Dim::Row, Dim::Z}, Shape{3, 2},
                                   units::Unit(units::m),
                                   Values{1, 2, 3, 4, 5, 6}));
}

TEST(VariableTest, reshape_with_variance) {
  const auto var = createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                          Values{1, 2, 3, 4, 5, 6},
                                          Variances{7, 8, 9, 10, 11, 12});

  ASSERT_EQ(var.reshape({Dim::Row, 6}),
            createVariable<double>(Dims{Dim::Row}, Shape{6},
                                   Values{1, 2, 3, 4, 5, 6},
                                   Variances{7, 8, 9, 10, 11, 12}));
  ASSERT_EQ(var.reshape({{Dim::Row, 3}, {Dim::Z, 2}}),
            createVariable<double>(Dims{Dim::Row, Dim::Z}, Shape{3, 2},
                                   Values{1, 2, 3, 4, 5, 6},
                                   Variances{7, 8, 9, 10, 11, 12}));
}

TEST(VariableTest, reshape_temporary) {
  const auto var = createVariable<double>(
      Dims{Dim::X, Dim::Row}, Shape{2, 4}, units::Unit(units::m),
      Values{1, 2, 3, 4, 5, 6, 7, 8}, Variances{9, 10, 11, 12, 13, 14, 15, 16});
  auto reshaped = sum(var, Dim::X).reshape({{Dim::Y, 2}, {Dim::Z, 2}});
  ASSERT_EQ(reshaped,
            createVariable<double>(Dims{Dim::Y, Dim::Z}, Shape{2, 2},
                                   units::Unit(units::m), Values{6, 8, 10, 12},
                                   Variances{22, 24, 26, 28}));

  // This is not a temporary, we get a view into `var`.
  EXPECT_EQ(typeid(decltype(std::move(var).reshape({}))),
            typeid(VariableConstProxy));
}

TEST(VariableTest, reshape_fail) {
  auto var = createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                    Values{1, 2, 3, 4, 5, 6});
  EXPECT_THROW_MSG(var.reshape({Dim::Row, 5}), std::runtime_error,
                   "Cannot reshape to dimensions with different volume");
}

TEST(VariableTest, reshape_and_slice) {
  auto var = createVariable<double>(
      Dims{Dim::Z}, Shape{16},
      Values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
      Variances{17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
                32});

  auto slice = var.reshape({{Dim::X, 4}, {Dim::Y, 4}})
                   .slice({Dim::X, 1, 3})
                   .slice({Dim::Y, 1, 3});
  ASSERT_EQ(slice, createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                          Values{6, 7, 10, 11},
                                          Variances{22, 23, 26, 27}));

  Variable center = var.reshape({{Dim::X, 4}, {Dim::Y, 4}})
                        .slice({Dim::X, 1, 3})
                        .slice({Dim::Y, 1, 3})
                        .reshape({Dim::Z, 4});

  ASSERT_EQ(center,
            createVariable<double>(Dims{Dim::Z}, Shape{4}, Values{6, 7, 10, 11},
                                   Variances{22, 23, 26, 27}));
}

TEST(VariableTest, reshape_mutable) {
  auto modified_original = createVariable<double>(
      Dims{Dim::X, Dim::Y}, Shape{2, 3}, Values{1, 2, 3, 0, 5, 6});
  auto reference = createVariable<double>(Dims{Dim::Row}, Shape{6},
                                          Values{1, 2, 3, 0, 5, 6});

  auto var = createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                    Values{1, 2, 3, 4, 5, 6});

  auto view = var.reshape({Dim::Row, 6});
  view.values<double>()[3] = 0;

  ASSERT_EQ(view, reference);
  ASSERT_EQ(var, modified_original);
}

TEST(VariableTest, rename) {
  auto var = createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                    Values{1, 2, 3, 4, 5, 6},
                                    Variances{7, 8, 9, 10, 11, 12});
  const Variable expected(var.reshape({{Dim::X, 2}, {Dim::Z, 3}}));

  var.rename(Dim::Y, Dim::Z);
  ASSERT_EQ(var, expected);
}

TEST(Variable, access_typed_view) {
  auto var = createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                    Values{1, 2, 3, 4, 5, 6});
  const auto values = dynamic_cast<const VariableConceptT<double> &>(var.data())
                          .valuesView({{Dim::Y, 2}, {Dim::Z, 4}, {Dim::X, 3}});
  ASSERT_EQ(values.size(), 24);

  for (const auto z : {0, 1, 2, 3}) {
    EXPECT_EQ(values[3 * z + 0], 1);
    EXPECT_EQ(values[3 * z + 1], 2);
    EXPECT_EQ(values[3 * z + 2], 3);
  }
  for (const auto z : {0, 1, 2, 3}) {
    EXPECT_EQ(values[12 + 3 * z + 0], 4);
    EXPECT_EQ(values[12 + 3 * z + 1], 5);
    EXPECT_EQ(values[12 + 3 * z + 2], 6);
  }
}

TEST(Variable, access_typed_view_edges) {
  // If a variable contains bin edges we want to "skip" the last edge. Say bins
  // is in direction Y:
  auto var = createVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                    Values{1, 2, 3, 4, 5, 6});
  const auto values = dynamic_cast<const VariableConceptT<double> &>(var.data())
                          .valuesView({{Dim::Y, 2}, {Dim::Z, 4}, {Dim::X, 2}});
  ASSERT_EQ(values.size(), 16);

  for (const auto z : {0, 1, 2, 3}) {
    EXPECT_EQ(values[2 * z + 0], 1);
    EXPECT_EQ(values[2 * z + 1], 4);
  }
  for (const auto z : {0, 1, 2, 3}) {
    EXPECT_EQ(values[8 + 2 * z + 0], 2);
    EXPECT_EQ(values[8 + 2 * z + 1], 5);
  }
}

TEST(SparseVariable, create) {
  const auto var =
      makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  EXPECT_TRUE(var.dims().sparse());
  EXPECT_EQ(var.dims().sparseDim(), Dim::X);
  // Should we return the full volume here, i.e., accumulate the extents of all
  // the sparse subdata?
  EXPECT_EQ(var.dims().volume(), 2);
}

TEST(SparseVariable, dtype) {
  const auto var =
      makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  // It is not clear that this is the best way of handling things.
  // Variable::dtype() makes sense like this, but it is not so clear for
  // VariableConcept::dtype().
  EXPECT_EQ(var.dtype(), dtype<double>);
  EXPECT_NE(var.data().dtype(), dtype<double>);
}

TEST(SparseVariable, non_sparse_access_fail) {
  const auto var = makeVariableWithVariances<double>(
      {{Dim::Y, Dim::X}, {2, Dimensions::Sparse}});
  ASSERT_THROW(var.values<double>(), except::TypeError);
  ASSERT_THROW(var.variances<double>(), except::TypeError);
}

TEST(SparseVariable, DISABLED_low_level_access) {
  const auto var =
      makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  // Need to decide whether we allow this direct access or not.
  ASSERT_THROW((var.values<sparse_container<double>>()), except::TypeError);
}

TEST(SparseVariable, access) {
  const auto var = makeVariableWithVariances<double>(
      {{Dim::Y, Dim::X}, {2, Dimensions::Sparse}});
  ASSERT_NO_THROW(var.sparseValues<double>());
  ASSERT_NO_THROW(var.sparseVariances<double>());
  const auto values = var.sparseValues<double>();
  const auto variances = var.sparseVariances<double>();
  ASSERT_EQ(values.size(), 2);
  EXPECT_TRUE(values[0].empty());
  EXPECT_TRUE(values[1].empty());
  ASSERT_EQ(variances.size(), 2);
  EXPECT_TRUE(variances[0].empty());
  EXPECT_TRUE(variances[1].empty());
}

TEST(SparseVariable, resize_sparse) {
  auto var = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto data = var.sparseValues<double>();
  data[1] = {1, 2, 3};
}

TEST(SparseVariable, copy) {
  const auto a = make_sparse_var_2d_with_variances();

  Variable copy(a);
  EXPECT_EQ(a, copy);
}

TEST(SparseVariable, move) {
  auto a = make_sparse_var_2d_with_variances();

  Variable copy(a);
  Variable moved(std::move(copy));
  EXPECT_EQ(a, moved);
}

TEST(SparseVariable, slice) {
  auto var = makeVariable<double>({Dim::Y, Dim::X}, {4, Dimensions::Sparse});
  auto data = var.sparseValues<double>();
  data[0] = {1, 2, 3};
  data[1] = {1, 2};
  data[2] = {1};
  data[3] = {};
  auto slice = var.slice({Dim::Y, 1, 3});
  EXPECT_TRUE(slice.dims().sparse());
  EXPECT_EQ(slice.dims().sparseDim(), Dim::X);
  EXPECT_EQ(slice.dims().volume(), 2);
  auto slice_data = slice.sparseValues<double>();
  EXPECT_TRUE(equals(slice_data[0], {1, 2}));
  EXPECT_TRUE(equals(slice_data[1], {1}));
}

TEST(SparseVariable, slice_fail) {
  auto var = makeVariable<double>({Dim::Y, Dim::X}, {4, Dimensions::Sparse});
  auto data = var.sparseValues<double>();
  data[0] = {1, 2, 3};
  data[1] = {1, 2};
  data[2] = {1};
  data[3] = {};
  ASSERT_THROW(var.slice({Dim::X, 0}), except::DimensionNotFoundError);
  ASSERT_THROW(var.slice({Dim::X, 0, 1}), except::DimensionNotFoundError);
}

TEST(VariableTest, create_with_variance) {
  ASSERT_NO_THROW(makeVariable<double>(1.0, 0.1));
  ASSERT_NO_THROW(createVariable<double>(Dims(), Shape(), units::Unit(units::m),
                                         Values{1.0}, Variances{0.1}));
}

TEST(VariableTest, hasVariances) {
  ASSERT_FALSE(makeVariable<double>({}).hasVariances());
  ASSERT_FALSE(makeVariable<double>(1.0).hasVariances());
  ASSERT_TRUE(makeVariable<double>(1.0, 0.1).hasVariances());
  ASSERT_TRUE(createVariable<double>(Dims(), Shape(), units::Unit(units::m),
                                     Values{1.0}, Variances{0.1})
                  .hasVariances());
}

TEST(VariableTest, values_variances) {
  const auto var = makeVariable<double>(1.0, 0.1);
  ASSERT_NO_THROW(var.values<double>());
  ASSERT_NO_THROW(var.variances<double>());
  ASSERT_TRUE(equals(var.values<double>(), {1.0}));
  ASSERT_TRUE(equals(var.variances<double>(), {0.1}));
}

template <typename Var> void test_set_variances(Var &var) {
  var.setVariances(Vector<double>{5.0, 6.0, 7.0});
  ASSERT_TRUE(equals(var.template variances<double>(), {5.0, 6.0, 7.0}));
  var.setVariances(Vector<double>{1.0, 2.0, 3.0});
  ASSERT_TRUE(equals(var.template variances<double>(), {1.0, 2.0, 3.0}));
  EXPECT_THROW(var.setVariances(Vector<double>{1.0, 2.0, 3.0, 4.0}),
               except::SizeError);
  EXPECT_NO_THROW(var.setVariances(Vector<float>{1.0, 2.0, 3.0}));
}

TEST(VariableTest, set_variances) {
  Variable var = createVariable<double>(
      Dims{Dim::X}, Shape{3}, units::Unit(units::m), Values{1.0, 2.0, 3.0});
  test_set_variances(var);
}

TEST(VariableProxyTest, set_variances) {
  Variable var = createVariable<double>(
      Dims{Dim::X}, Shape{3}, units::Unit(units::m), Values{1.0, 2.0, 3.0});
  auto proxy = VariableProxy(var);
  test_set_variances(proxy);
}

TEST(VariableProxyTest, create_with_variance) {
  const auto var = createVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{1.0, 2.0}, Variances{0.1, 0.2});
  ASSERT_NO_THROW(var.slice({Dim::X, 1, 2}));
  const auto slice = var.slice({Dim::X, 1, 2});
  ASSERT_TRUE(slice.hasVariances());
  ASSERT_EQ(slice.variances<double>().size(), 1);
  ASSERT_EQ(slice.variances<double>()[0], 0.2);
  const auto reference = createVariable<double>(Dims{Dim::X}, Shape{1},
                                                Values{2.0}, Variances{0.2});
  ASSERT_EQ(slice, reference);
}

TEST(VariableTest, variances_unsupported_type_fail) {
  ASSERT_NO_THROW(
      createVariable<std::string>(Dims{Dim::X}, Shape{1}, Values{"a"}));
  ASSERT_THROW(createVariable<std::string>(Dims{Dim::X}, Shape{1}, Values{"a"},
                                           Variances{"variances"}),
               except::VariancesError);
}

TEST(VariableTest, construct_proxy_dims) {
  auto var = makeVariable<double>({Dim::Y, Dim::X}, {2, 3});
  Variable vv(var.slice({Dim::X, 0, 2}));
  ASSERT_NO_THROW(Variable(var.slice({Dim::X, 0, 2}), Dimensions(Dim::Y, 2)));
}

TEST(VariableTest, construct_mult_dev_unit) {
  Variable refDiv = createVariable<float>(
      Dims(), Shape(),
      units::Unit(units::dimensionless) / units::Unit(units::m), Values{1.0f});
  Variable refMult = createVariable<int32_t>(Dims(), Shape(),
                                             units::Unit(units::kg), Values{1});
  EXPECT_EQ(1.0f / units::Unit(units::m), refDiv);
  EXPECT_EQ(int32_t(1) * units::Unit(units::kg), refMult);
}

template <class T> class AsTypeTest : public ::testing::Test {};

using type_pairs =
    ::testing::Types<std::pair<float, double>, std::pair<double, float>,
                     std::pair<int32_t, float>>;
TYPED_TEST_CASE(AsTypeTest, type_pairs);

TYPED_TEST(AsTypeTest, variable_astype) {
  using T1 = typename TypeParam::first_type;
  using T2 = typename TypeParam::second_type;
  auto var1 = makeVariable<T1>(1, 1);
  auto var2 = makeVariable<T2>(1, 1);
  ASSERT_EQ(astype(var1, core::dtype<T2>), var2);
  var1 = makeVariable<T1>(1);
  var2 = makeVariable<T2>(1);
  ASSERT_EQ(astype(var1, core::dtype<T2>), var2);
  var1 = createVariable<T1>(Dims{Dim::X}, Shape{3}, units::Unit(units::m),
                            Values{1.0, 2.0, 3.0});
  var2 = createVariable<T2>(Dims{Dim::X}, Shape{3}, units::Unit(units::m),
                            Values{1.0, 2.0, 3.0});
  ASSERT_EQ(astype(var1, core::dtype<T2>), var2);
}