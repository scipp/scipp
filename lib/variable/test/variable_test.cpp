// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include <units/units.hpp>

#include "test_macros.h"

#include "scipp/core/eigen.h"
#include "scipp/core/except.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/variable.h"

using namespace scipp;

TEST(Variable, construct_default) {
  ASSERT_NO_THROW(Variable{});
  Variable var;
  ASSERT_FALSE(var.is_valid());
}

TEST(Variable, construct) {
  ASSERT_NO_THROW(makeVariable<double>(Dims{Dim::X}, Shape{2}));
  ASSERT_NO_THROW(makeVariable<double>(Dims{Dim::X}, Shape{2}, Values(2)));
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2});
  const auto &data = a.values<double>();
  EXPECT_EQ(data.size(), 2);
}

TEST(Variable, many_dims_works_or_fails_gracefully) {
  Dimensions dims;
  for (const auto dim : "abcdefghijklmn")
    dims.addInner(Dim(std::string(1, dim)), 1);
  auto var = makeVariable<double>(dims, Values{1});
  EXPECT_EQ(var.ndim(), 15);
  EXPECT_EQ(copy(var), var);
  EXPECT_EQ(var + var, makeVariable<double>(dims, Values{2}));
  EXPECT_EQ(var + 1.0 * sc_units::one, makeVariable<double>(dims, Values{2}));
  // TODO In principle we should be able to support all of the below with
  // flattening, but the current implementation dos not handle this.
  ASSERT_THROW(var +=
               makeVariable<double>(Dims{Dim("a")}, Shape{2}, Values{1, 2}),
               std::runtime_error);
  ASSERT_THROW(var +=
               makeVariable<double>(Dims{Dim("g")}, Shape{2}, Values{1, 2}),
               std::runtime_error);
  ASSERT_THROW(var +=
               makeVariable<double>(Dims{Dim("n")}, Shape{2}, Values{1, 2}),
               std::runtime_error);
}

TEST(Variable, default_unit_of_numeric_is_dimensionless) {
  EXPECT_EQ(makeVariable<double>(Dimensions{}).unit(), sc_units::one);
  EXPECT_EQ(makeVariable<float>(Dimensions{}).unit(), sc_units::one);
  EXPECT_EQ(makeVariable<int64_t>(Dimensions{}).unit(), sc_units::one);
  EXPECT_EQ(makeVariable<int32_t>(Dimensions{}).unit(), sc_units::one);
}

TEST(Variable, default_unit_of_bool_is_none) {
  EXPECT_EQ(makeVariable<bool>(Dimensions{}).unit(), sc_units::none);
}

TEST(Variable, default_unit_of_time_point_is_dimensionless) {
  EXPECT_EQ(makeVariable<core::time_point>(Dimensions{}).unit(), sc_units::one);
}

TEST(Variable, default_unit_of_spatial_types_is_dimensionless) {
  EXPECT_EQ(makeVariable<Eigen::Vector3d>(Dimensions{}).unit(), sc_units::one);
  EXPECT_EQ(makeVariable<Eigen::Matrix3d>(Dimensions{}).unit(), sc_units::one);
  EXPECT_EQ(makeVariable<Eigen::Affine3d>(Dimensions{}).unit(), sc_units::one);
  EXPECT_EQ(makeVariable<core::Translation>(Dimensions{}).unit(),
            sc_units::one);
  EXPECT_EQ(makeVariable<core::Quaternion>(Dimensions{}).unit(), sc_units::one);
}

TEST(Variable, default_unit_of_index_pair_is_none) {
  EXPECT_EQ(makeVariable<scipp::index_pair>(Dimensions{}).unit(),
            sc_units::none);
}

TEST(Variable, default_unit_of_string_is_none) {
  EXPECT_EQ(makeVariable<std::string>(Dimensions{}).unit(), sc_units::none);
}

TEST(Variable, construct_llnl_units_quantity) {
  EXPECT_EQ(Variable(1.2 * units::precise::meter),
            makeVariable<double>(Values{1.2}, sc_units::m));
  // llnl measurement is always double
  EXPECT_EQ(Variable(1.0f * units::precise::meter),
            makeVariable<double>(Values{1.0}, sc_units::m));
}

TEST(Variable, construct_fail) {
  ASSERT_ANY_THROW(makeVariable<double>(Dims{}, Shape{}, Values(2)));
  ASSERT_ANY_THROW(makeVariable<double>(Dims{Dim::X}, Shape{1}, Values(2)));
  ASSERT_ANY_THROW(makeVariable<double>(Dims{Dim::X}, Shape{3}, Values(2)));
}

TEST(Variable, copy) {
  const auto var =
      makeVariable<double>(Dimensions{Dim::X, 3}, Values{}, Variances{});
  const Variable view(var);
  EXPECT_EQ(var.unit(), view.unit());
  EXPECT_EQ(var.dims(), view.dims());
  EXPECT_EQ(var.values<double>().data(), view.values<double>().data());
  EXPECT_EQ(var.variances<double>().data(), view.variances<double>().data());
}

TEST(Variable, move) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{2});
  Variable moved(std::move(var));
  EXPECT_FALSE(var.is_valid());
  EXPECT_NE(moved, var);
}

TEST(Variable, is_readonly) {
  auto var = makeVariable<double>(Values{1});
  EXPECT_FALSE(var.is_readonly());
  EXPECT_FALSE(Variable(var).is_readonly()); // propagated on copy
  EXPECT_FALSE(copy(var).is_readonly());     // reset on deep copy
  auto const_var = var.as_const();
  EXPECT_TRUE(const_var.is_readonly());
  EXPECT_TRUE(Variable(const_var).is_readonly()); // propagated on copy
  EXPECT_FALSE(copy(const_var).is_readonly());    // reset on deep copy
}

TEST(Variable, is_aligned_default) {
  const auto var = makeVariable<double>(Values{1});
  EXPECT_TRUE(var.is_aligned());
}

TEST(Variable, can_set_aligned_flag) {
  auto var = makeVariable<double>(Values{1});
  var.set_aligned(false);
  EXPECT_FALSE(var.is_aligned());
  var.set_aligned(true);
  EXPECT_TRUE(var.is_aligned());
}

TEST(Variable, set_aligned_does_not_affect_copies) {
  auto var = makeVariable<double>(Values{1});
  auto shallow_copy = Variable(var);
  auto deep_copy = copy(var);
  var.set_aligned(false);
  EXPECT_TRUE(shallow_copy.is_aligned());
  EXPECT_TRUE(deep_copy.is_aligned());
}

TEST(Variable, alignment_copy_behavior) {
  auto var = makeVariable<double>(Values{1});
  EXPECT_TRUE(Variable(var).is_aligned());
  EXPECT_TRUE(copy(var).is_aligned());
  var.set_aligned(false);
  EXPECT_FALSE(Variable(var).is_aligned());
  EXPECT_FALSE(copy(var).is_aligned());
}

TEST(Variable, alignment_copy_assignment_behavior) {
  auto var1 = makeVariable<double>(Values{1});
  auto var2 = makeVariable<double>(Values{1});
  var2.set_aligned(false);
  var2 = var1;
  EXPECT_TRUE(var2.is_aligned());

  var1.set_aligned(false);
  var2.set_aligned(true);
  var2 = var1;
  EXPECT_FALSE(var2.is_aligned());
}

TEST(Variable, alignment_move_assignment_behavior) {
  auto var1 = makeVariable<double>(Values{1});
  auto var2 = makeVariable<double>(Values{1});
  var2.set_aligned(false);
  var2 = std::move(var1);
  EXPECT_TRUE(var2.is_aligned());

  auto var3 = makeVariable<double>(Values{1});
  var3.set_aligned(false);
  var2.set_aligned(true);
  var2 = std::move(var3);
  EXPECT_FALSE(var2.is_aligned());
}

TEST(Variable, is_valid) {
  auto a = Variable();
  EXPECT_FALSE(a.is_valid());
  a = makeVariable<double>(Values{1});
  EXPECT_TRUE(a.is_valid());
}

TEST(Variable, is_slice) {
  auto var = makeVariable<double>(Dims{Dim::X}, Values{1, 2, 3}, Shape{3});
  EXPECT_FALSE(var.is_slice());
  EXPECT_FALSE(var.slice({Dim::X, 0, 3}).is_slice());
  EXPECT_TRUE(var.slice({Dim::X, 1, 3}).is_slice());
  EXPECT_TRUE(var.slice({Dim::X, 0, 1}).is_slice());
}

TEST(Variable, is_same) {
  auto a = makeVariable<double>(Dims{Dim::X}, Values{1, 2}, Shape{2});
  EXPECT_TRUE(a.is_same(Variable(a)));
  EXPECT_TRUE(a.is_same(a.as_const()));
  EXPECT_FALSE(a.is_same(a.slice({Dim::X, 0, 1})));

  auto b = makeVariable<double>(Dims{Dim::Y, Dim::X}, Values{1, 2, 3, 4},
                                Shape{2, 2});
  EXPECT_FALSE(b.is_same(b.transpose(std::vector<Dim>{Dim::X, Dim::Y})));
}

TEST(Variable, makeVariable_custom_type) {
  auto doubles = makeVariable<double>(Values{double{}});
  auto floats = makeVariable<float>(Values{float{}});

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
  auto doubles = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  auto ints = makeVariable<int32_t>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});

  // Passed ints but uses default type based on tag.
  ASSERT_NO_THROW(doubles.values<double>());
  // Passed doubles but explicit type overrides.
  ASSERT_NO_THROW(ints.values<int32_t>());
}

TEST(Variable, dtype) {
  auto doubles = makeVariable<double>(Values{double{}});
  auto floats = makeVariable<float>(Values{float{}});
  EXPECT_EQ(doubles.dtype(), dtype<double>);
  EXPECT_NE(doubles.dtype(), dtype<float>);
  EXPECT_NE(floats.dtype(), dtype<double>);
  EXPECT_EQ(floats.dtype(), dtype<float>);
  EXPECT_EQ(doubles.dtype(), doubles.dtype());
  EXPECT_EQ(floats.dtype(), floats.dtype());
  EXPECT_NE(doubles.dtype(), floats.dtype());
}

TEST(Variable, span_references_Variable) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2});
  auto observer = std::as_const(a).values<double>();
  // This line does not compile, const-correctness works:
  // observer[0] = 1.0;

  auto span = a.values<double>();

  EXPECT_EQ(span.size(), 2);
  // cppcheck-suppress unreadVariable  # Read through `observer`.
  span[0] = 1.0;
  EXPECT_EQ(observer[0], 1.0);
}

TEST(VariableTest, copy_and_move) {
  const auto reference =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1}, sc_units::m,
                           Values{1.1, 2.2}, Variances{0.1, 0.2});
  const auto var =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1}, sc_units::m,
                           Values{1.1, 2.2}, Variances{0.1, 0.2});

  const Variable shallow(var);
  EXPECT_EQ(shallow, reference);

  const auto deep = copy(var);
  EXPECT_EQ(deep, reference);

  const auto moved(std::move(var));
  EXPECT_EQ(moved, reference);
}

TEST(Variable, full_slice) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2});
  EXPECT_TRUE(var.is_same(var.slice({})));
}

TEST(Variable, copy_slice) {
  const auto parent = makeVariable<double>(
      Dims{Dim::X, Dim::Y, Dim::Z}, Shape{4, 2, 3},
      Values{1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
             13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},
      Variances{25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
                37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48});
  const auto empty = makeVariable<double>(
      Dimensions{{Dim::X, 4}, {Dim::Y, 2}, {Dim::Z, 3}}, Values{}, Variances{});

  auto d = copy(empty);
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1, 2, 3})
    copy(parent.slice({Dim::X, index}), d.slice({Dim::X, index}));
  EXPECT_EQ(parent, d);

  d = copy(empty);
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1})
    copy(parent.slice({Dim::Y, index}), d.slice({Dim::Y, index}));
  EXPECT_EQ(parent, d);

  d = copy(empty);
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1, 2})
    copy(parent.slice({Dim::Z, index}), d.slice({Dim::Z, index}));
  EXPECT_EQ(parent, d);
}

TEST(Variable, copy_slice_unit_checks) {
  const auto parent =
      makeVariable<double>(Dims(), Shape(), sc_units::m, Values{1});
  auto dimensionless = makeVariable<double>(Dims{Dim::X}, Shape{4});
  auto m = makeVariable<double>(Dims{Dim::X}, Shape{4}, sc_units::m);

  EXPECT_THROW(copy(parent, dimensionless.slice({Dim::X, 1})),
               except::UnitError);
  EXPECT_NO_THROW(copy(parent, m.slice({Dim::X, 1})));
}

TEST(Variable, copy_slice_variance_checks) {
  const auto parent_vals = makeVariable<double>(Values{1.0});
  const auto parent_vals_vars =
      makeVariable<double>(Values{1.0}, Variances{2.0});
  auto vals = makeVariable<double>(Dims{Dim::X}, Shape{4});
  auto vals_vars =
      makeVariable<double>(Dimensions{Dim::X, 4}, Values{}, Variances{});

  EXPECT_NO_THROW(copy(parent_vals, vals.slice({Dim::X, 1})));
  EXPECT_NO_THROW(copy(parent_vals_vars, vals_vars.slice({Dim::X, 1})));
  EXPECT_THROW(copy(parent_vals_vars, vals.slice({Dim::X, 1})),
               except::VariancesError);
  EXPECT_THROW(copy(parent_vals, vals_vars.slice({Dim::X, 1})),
               except::VariancesError);
}

class VariableTest_3d : public ::testing::Test {
protected:
  const Variable parent{makeVariable<double>(
      Dims{Dim::X, Dim::Y, Dim::Z}, Shape{4, 2, 3}, sc_units::m,
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
            makeVariable<double>(Dimensions(dims_no_x), sc_units::m,
                                 Values(vals_x0.begin(), vals_x0.end()),
                                 Variances(vars_x0.begin(), vars_x0.end())));
  EXPECT_EQ(parent.slice({Dim::X, 1}),
            makeVariable<double>(Dimensions(dims_no_x), sc_units::m,
                                 Values(vals_x1.begin(), vals_x1.end()),
                                 Variances(vars_x1.begin(), vars_x1.end())));
  EXPECT_EQ(parent.slice({Dim::X, 2}),
            makeVariable<double>(Dimensions(dims_no_x), sc_units::m,
                                 Values(vals_x2.begin(), vals_x2.end()),
                                 Variances(vars_x2.begin(), vars_x2.end())));
  EXPECT_EQ(parent.slice({Dim::X, 3}),
            makeVariable<double>(Dimensions(dims_no_x), sc_units::m,
                                 Values(vals_x3.begin(), vals_x3.end()),
                                 Variances(vars_x3.begin(), vars_x3.end())));

  Dimensions dims_no_y{{Dim::X, 4}, {Dim::Z, 3}};
  EXPECT_EQ(parent.slice({Dim::Y, 0}),
            makeVariable<double>(Dimensions(dims_no_y), sc_units::m,
                                 Values(vals_y0.begin(), vals_y0.end()),
                                 Variances(vars_y0.begin(), vars_y0.end())));
  EXPECT_EQ(parent.slice({Dim::Y, 1}),
            makeVariable<double>(Dimensions(dims_no_y), sc_units::m,
                                 Values(vals_y1.begin(), vals_y1.end()),
                                 Variances(vars_y1.begin(), vars_y1.end())));

  Dimensions dims_no_z{{Dim::X, 4}, {Dim::Y, 2}};
  EXPECT_EQ(parent.slice({Dim::Z, 0}),
            makeVariable<double>(Dimensions(dims_no_z), sc_units::m,
                                 Values(vals_z0.begin(), vals_z0.end()),
                                 Variances(vars_z0.begin(), vars_z0.end())));
  EXPECT_EQ(parent.slice({Dim::Z, 1}),
            makeVariable<double>(Dimensions(dims_no_z), sc_units::m,
                                 Values(vals_z1.begin(), vals_z1.end()),
                                 Variances(vars_z1.begin(), vars_z1.end())));
  EXPECT_EQ(parent.slice({Dim::Z, 2}),
            makeVariable<double>(Dimensions(dims_no_z), sc_units::m,
                                 Values(vals_z2.begin(), vals_z2.end()),
                                 Variances(vars_z2.begin(), vars_z2.end())));
}

TEST_F(VariableTest_3d, slice_range) {
  // Length 1 slice
  Dimensions dims_x1{{Dim::X, 1}, {Dim::Y, 2}, {Dim::Z, 3}};
  EXPECT_EQ(parent.slice({Dim::X, 0, 1}),
            makeVariable<double>(Dimensions(dims_x1), sc_units::m,
                                 Values(vals_x0.begin(), vals_x0.end()),
                                 Variances(vars_x0.begin(), vars_x0.end())));
  EXPECT_EQ(parent.slice({Dim::X, 1, 2}),
            makeVariable<double>(Dimensions(dims_x1), sc_units::m,
                                 Values(vals_x1.begin(), vals_x1.end()),
                                 Variances(vars_x1.begin(), vars_x1.end())));
  EXPECT_EQ(parent.slice({Dim::X, 2, 3}),
            makeVariable<double>(Dimensions(dims_x1), sc_units::m,
                                 Values(vals_x2.begin(), vals_x2.end()),
                                 Variances(vars_x2.begin(), vars_x2.end())));
  EXPECT_EQ(parent.slice({Dim::X, 3, 4}),
            makeVariable<double>(Dimensions(dims_x1), sc_units::m,
                                 Values(vals_x3.begin(), vals_x3.end()),
                                 Variances(vars_x3.begin(), vars_x3.end())));

  Dimensions dims_y1{{Dim::X, 4}, {Dim::Y, 1}, {Dim::Z, 3}};
  EXPECT_EQ(parent.slice({Dim::Y, 0, 1}),
            makeVariable<double>(Dimensions(dims_y1), sc_units::m,
                                 Values(vals_y0.begin(), vals_y0.end()),
                                 Variances(vars_y0.begin(), vars_y0.end())));
  EXPECT_EQ(parent.slice({Dim::Y, 1, 2}),
            makeVariable<double>(Dimensions(dims_y1), sc_units::m,
                                 Values(vals_y1.begin(), vals_y1.end()),
                                 Variances(vars_y1.begin(), vars_y1.end())));

  Dimensions dims_z1{{Dim::X, 4}, {Dim::Y, 2}, {Dim::Z, 1}};
  EXPECT_EQ(parent.slice({Dim::Z, 0, 1}),
            makeVariable<double>(Dimensions(dims_z1), sc_units::m,
                                 Values(vals_z0.begin(), vals_z0.end()),
                                 Variances(vars_z0.begin(), vars_z0.end())));
  EXPECT_EQ(parent.slice({Dim::Z, 1, 2}),
            makeVariable<double>(Dimensions(dims_z1), sc_units::m,
                                 Values(vals_z1.begin(), vals_z1.end()),
                                 Variances(vars_z1.begin(), vars_z1.end())));
  EXPECT_EQ(parent.slice({Dim::Z, 2, 3}),
            makeVariable<double>(Dimensions(dims_z1), sc_units::m,
                                 Values(vals_z2.begin(), vals_z2.end()),
                                 Variances(vars_z2.begin(), vars_z2.end())));

  // Length 2 slice
  Dimensions dims_x2{{Dim::X, 2}, {Dim::Y, 2}, {Dim::Z, 3}};
  EXPECT_EQ(parent.slice({Dim::X, 0, 2}),
            makeVariable<double>(Dimensions(dims_x2), sc_units::m,
                                 Values(vals_x02.begin(), vals_x02.end()),
                                 Variances(vars_x02.begin(), vars_x02.end())));
  EXPECT_EQ(parent.slice({Dim::X, 1, 3}),
            makeVariable<double>(Dimensions(dims_x2), sc_units::m,
                                 Values(vals_x13.begin(), vals_x13.end()),
                                 Variances(vars_x13.begin(), vars_x13.end())));
  EXPECT_EQ(parent.slice({Dim::X, 2, 4}),
            makeVariable<double>(Dimensions(dims_x2), sc_units::m,
                                 Values(vals_x24.begin(), vals_x24.end()),
                                 Variances(vars_x24.begin(), vars_x24.end())));

  EXPECT_EQ(parent.slice({Dim::Y, 0, 2}), parent);

  Dimensions dims_z2{{Dim::X, 4}, {Dim::Y, 2}, {Dim::Z, 2}};
  EXPECT_EQ(parent.slice({Dim::Z, 0, 2}),
            makeVariable<double>(Dimensions(dims_z2), sc_units::m,
                                 Values(vals_z02.begin(), vals_z02.end()),
                                 Variances(vars_z02.begin(), vars_z02.end())));
  EXPECT_EQ(parent.slice({Dim::Z, 1, 3}),
            makeVariable<double>(Dimensions(dims_z2), sc_units::m,
                                 Values(vals_z13.begin(), vals_z13.end()),
                                 Variances(vars_z13.begin(), vars_z13.end())));
}

TEST(VariableView, strides) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
  EXPECT_TRUE(equals(var.slice({Dim::X, 0}).strides(), {3}));
  EXPECT_TRUE(equals(var.slice({Dim::X, 1}).strides(), {3}));
  EXPECT_TRUE(equals(var.slice({Dim::Y, 0}).strides(), {1}));
  EXPECT_TRUE(equals(var.slice({Dim::Y, 1}).strides(), {1}));
  EXPECT_TRUE(equals(var.slice({Dim::X, 0, 1}).strides(), {3, 1}));
  EXPECT_TRUE(equals(var.slice({Dim::X, 1, 2}).strides(), {3, 1}));
  EXPECT_TRUE(equals(var.slice({Dim::Y, 0, 1}).strides(), {3, 1}));
  EXPECT_TRUE(equals(var.slice({Dim::Y, 1, 2}).strides(), {3, 1}));
  EXPECT_TRUE(equals(var.slice({Dim::X, 0, 2}).strides(), {3, 1}));
  EXPECT_TRUE(equals(var.slice({Dim::X, 1, 3}).strides(), {3, 1}));
  EXPECT_TRUE(equals(var.slice({Dim::Y, 0, 2}).strides(), {3, 1}));
  EXPECT_TRUE(equals(var.slice({Dim::Y, 1, 3}).strides(), {3, 1}));

  EXPECT_TRUE(equals(var.slice({Dim::X, 0, 1}).slice({Dim::Y, 0, 1}).strides(),
                     {3, 1}));

  auto var3D =
      makeVariable<double>(Dims{Dim::Z, Dim::Y, Dim::X}, Shape{4, 3, 2});
  EXPECT_TRUE(
      equals(var3D.slice({Dim::X, 0, 1}).slice({Dim::Z, 0, 1}).strides(),
             std::vector<scipp::index>{6, 2, 1}));
}

TEST(VariableView, stride) {
  auto var = makeVariable<double>(Dims{Dim::Z, Dim::Y, Dim::X}, Shape{2, 3, 4});
  EXPECT_EQ(var.stride(Dim::X), 1);
  EXPECT_EQ(var.stride(Dim::Y), 4);
  EXPECT_EQ(var.stride(Dim::Z), 12);
  var = transpose(var);
  EXPECT_EQ(var.stride(Dim::X), 1);
  EXPECT_EQ(var.stride(Dim::Y), 4);
  EXPECT_EQ(var.stride(Dim::Z), 12);
  var = copy(var);
  EXPECT_EQ(var.stride(Dim::X), 6);
  EXPECT_EQ(var.stride(Dim::Y), 2);
  EXPECT_EQ(var.stride(Dim::Z), 1);
}

TEST(VariableView, values_and_variances) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3},
                                        Variances{4, 5, 6});
  const auto view = var.slice({Dim::X, 1, 2});
  EXPECT_EQ(view.values<double>().size(), 1);
  EXPECT_EQ(view.values<double>()[0], 2.0);
  EXPECT_EQ(view.variances<double>().size(), 1);
  EXPECT_EQ(view.variances<double>()[0], 5.0);
}

TEST(VariableView, slicing_does_not_transpose) {
  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 3});
  Dimensions expected{{Dim::X, 1}, {Dim::Y, 1}};
  EXPECT_EQ(var.slice({Dim::X, 1, 2}).slice({Dim::Y, 1, 2}).dims(), expected);
  EXPECT_EQ(var.slice({Dim::Y, 1, 2}).slice({Dim::X, 1, 2}).dims(), expected);
}

TEST(VariableView, variable_copy_from_slice) {
  const auto source =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3}, sc_units::m,
                           Values{11, 12, 13, 21, 22, 23, 31, 32, 33},
                           Variances{44, 45, 46, 54, 55, 56, 64, 65, 66});

  const Dimensions dims({{Dim::Y, 2}, {Dim::X, 2}});
  EXPECT_EQ(copy(source.slice({Dim::X, 0, 2}).slice({Dim::Y, 0, 2})),
            makeVariable<double>(Dimensions(dims), sc_units::m,
                                 Values{11, 12, 21, 22},
                                 Variances{44, 45, 54, 55}));

  EXPECT_EQ(copy(source.slice({Dim::X, 1, 3}).slice({Dim::Y, 0, 2})),
            makeVariable<double>(Dimensions(dims), sc_units::m,
                                 Values{12, 13, 22, 23},
                                 Variances{45, 46, 55, 56}));

  EXPECT_EQ(copy(source.slice({Dim::X, 0, 2}).slice({Dim::Y, 1, 3})),
            makeVariable<double>(Dimensions(dims), sc_units::m,
                                 Values{21, 22, 31, 32},
                                 Variances{54, 55, 64, 65}));

  EXPECT_EQ(copy(source.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3})),
            makeVariable<double>(Dimensions(dims), sc_units::m,
                                 Values{22, 23, 32, 33},
                                 Variances{55, 56, 65, 66}));
}

TEST(VariableView, variable_assign_from_slice) {
  const Dimensions dims({{Dim::Y, 2}, {Dim::X, 2}});
  // Unit is dimensionless
  auto target = makeVariable<double>(Dimensions(dims), Values{1, 2, 3, 4},
                                     Variances{1, 2, 3, 4});
  const auto source =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3}, sc_units::m,
                           Values{11, 12, 13, 21, 22, 23, 31, 32, 33},
                           Variances{44, 45, 46, 54, 55, 56, 64, 65, 66});

  copy(source.slice({Dim::X, 0, 2}).slice({Dim::Y, 0, 2}), target);
  EXPECT_EQ(target, makeVariable<double>(Dimensions(dims), sc_units::m,
                                         Values{11, 12, 21, 22},
                                         Variances{44, 45, 54, 55}));

  copy(source.slice({Dim::X, 1, 3}).slice({Dim::Y, 0, 2}), target);
  EXPECT_EQ(target, makeVariable<double>(Dimensions(dims), sc_units::m,
                                         Values{12, 13, 22, 23},
                                         Variances{45, 46, 55, 56}));

  copy(source.slice({Dim::X, 0, 2}).slice({Dim::Y, 1, 3}), target);
  EXPECT_EQ(target, makeVariable<double>(Dimensions(dims), sc_units::m,
                                         Values{21, 22, 31, 32},
                                         Variances{54, 55, 64, 65}));

  copy(source.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3}), target);
  EXPECT_EQ(target, makeVariable<double>(Dimensions(dims), sc_units::m,
                                         Values{22, 23, 32, 33},
                                         Variances{55, 56, 65, 66}));
}

TEST(VariableView, variable_assign_from_slice_clears_variances) {
  const Dimensions dims({{Dim::Y, 2}, {Dim::X, 2}});
  auto target = makeVariable<double>(Dimensions(dims), Values{1, 2, 3, 4},
                                     Variances{5, 6, 7, 8});
  const auto source =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3}, sc_units::m,
                           Values{11, 12, 13, 21, 22, 23, 31, 32, 33});

  target = source.slice({Dim::X, 0, 2}).slice({Dim::Y, 0, 2});
  EXPECT_EQ(target, makeVariable<double>(Dimensions(dims), sc_units::m,
                                         Values{11, 12, 21, 22}));
}

TEST(VariableView, slice_copy_from_variable_broadcast) {
  const auto source = makeVariable<double>(Values{2});
  auto target = makeVariable<double>(Dims{Dim::X}, Shape{3});
  copy(source, target.slice({Dim::X, 1, 3}));
  EXPECT_EQ(target, makeVariable<double>(target.dims(), Values{0, 2, 2}));
}

TEST(VariableView, variable_self_assign_via_slice) {
  auto target =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3},
                           Values{11, 12, 13, 21, 22, 23, 31, 32, 33},
                           Variances{44, 45, 46, 54, 55, 56, 64, 65, 66});

  target = target.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3});
  // Note: This test does not actually fail if self-assignment is broken. Had to
  // run address sanitizer to see that it is reading from free'ed memory.
  EXPECT_EQ(target, makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                         Values{22, 23, 32, 33},
                                         Variances{55, 56, 65, 66}));
}

TEST(VariableView, slice_copy_from_variable_unit_fail) {
  const auto source = makeVariable<double>(Dims{Dim::X}, Shape{1}, sc_units::m);
  auto target = makeVariable<double>(Dims{Dim::X}, Shape{2});
  EXPECT_THROW(copy(source, target.slice({Dim::X, 1, 2})), except::UnitError);
  target = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m);
  EXPECT_NO_THROW(copy(source, target.slice({Dim::X, 1, 2})));
}

TEST(VariableView, slice_copy_from_variable_dimension_fail) {
  const auto source = makeVariable<double>(Dims{Dim::Y}, Shape{1});
  auto target = makeVariable<double>(Dims{Dim::X}, Shape{2});
  EXPECT_THROW(copy(source, target.slice({Dim::X, 1, 2})),
               except::DimensionError);
}

TEST(VariableView, slice_copy_from_variable_full_slice_can_change_unit) {
  const auto source = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m);
  auto target = makeVariable<double>(Dims{Dim::X}, Shape{2});
  copy(source, target.slice({Dim::X, 0, 2}));
  EXPECT_NO_THROW(copy(source, target.slice({Dim::X, 0, 2})));
}

TEST(VariableView, slice_copy_from_variable_variance_fail) {
  const auto vals = makeVariable<double>(Dims{Dim::X}, Shape{1});
  const auto vals_vars =
      makeVariable<double>(Dimensions{Dim::X, 1}, Values{}, Variances{});

  auto target = makeVariable<double>(Dims{Dim::X}, Shape{2});
  EXPECT_THROW(copy(vals_vars, target.slice({Dim::X, 1, 2})),
               except::VariancesError);
  EXPECT_NO_THROW(copy(vals, target.slice({Dim::X, 1, 2})));

  target = makeVariable<double>(Dimensions{Dim::X, 2}, Values{}, Variances{});
  EXPECT_THROW(copy(vals, target.slice({Dim::X, 1, 2})),
               except::VariancesError);
  EXPECT_NO_THROW(copy(vals_vars, target.slice({Dim::X, 1, 2})));
}

TEST(VariableView, slice_copy_from_variable) {
  const auto source =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                           Values{11, 12, 21, 22}, Variances{33, 34, 43, 44});

  // We might want to mimic Python's __setitem__, but operator= would (and
  // should!?) assign the view contents, not the data.
  const Dimensions dims({{Dim::Y, 3}, {Dim::X, 3}});
  auto target = makeVariable<double>(Dimensions{dims}, Values{}, Variances{});
  copy(source, target.slice({Dim::X, 0, 2}).slice({Dim::Y, 0, 2}));
  EXPECT_EQ(target, makeVariable<double>(
                        Dimensions(dims), Values{11, 12, 0, 21, 22, 0, 0, 0, 0},
                        Variances{33, 34, 0, 43, 44, 0, 0, 0, 0}));

  target = makeVariable<double>(Dimensions{dims}, Values{}, Variances{});
  copy(source, target.slice({Dim::X, 1, 3}).slice({Dim::Y, 0, 2}));
  EXPECT_EQ(target, makeVariable<double>(
                        Dimensions(dims), Values{0, 11, 12, 0, 21, 22, 0, 0, 0},
                        Variances{0, 33, 34, 0, 43, 44, 0, 0, 0}));

  target = makeVariable<double>(Dimensions{dims}, Values{}, Variances{});
  copy(source, target.slice({Dim::X, 0, 2}).slice({Dim::Y, 1, 3}));
  EXPECT_EQ(target, makeVariable<double>(
                        Dimensions(dims), Values{0, 0, 0, 11, 12, 0, 21, 22, 0},
                        Variances{0, 0, 0, 33, 34, 0, 43, 44, 0}));

  target = makeVariable<double>(Dimensions{dims}, Values{}, Variances{});
  copy(source, target.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3}));
  EXPECT_EQ(target, makeVariable<double>(
                        Dimensions(dims), Values{0, 0, 0, 0, 11, 12, 0, 21, 22},
                        Variances{0, 0, 0, 0, 33, 34, 0, 43, 44}));
}

TEST(VariableTest, rename_dims) {
  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                  Values{1, 2, 3, 4, 5, 6},
                                  Variances{7, 8, 9, 10, 11, 12});
  const auto expected = makeVariable<double>(Dims{Dim::X, Dim::Z}, Shape{2, 3},
                                             Values{1, 2, 3, 4, 5, 6},
                                             Variances{7, 8, 9, 10, 11, 12});

  const auto view = var.rename_dims({{Dim::Y, Dim::Z}});
  ASSERT_EQ(view, expected);
  ASSERT_EQ(view.slice({Dim::X, 1}), expected.slice({Dim::X, 1}));
  ASSERT_EQ(view.slice({Dim::Z, 1}), expected.slice({Dim::Z, 1}));
  ASSERT_NE(var, expected);
}

TEST(VariableTest, create_with_variance) {
  ASSERT_NO_THROW(makeVariable<double>(Values{1.0}, Variances{0.1}));
  ASSERT_NO_THROW(makeVariable<double>(Dims(), Shape(), sc_units::m,
                                       Values{1.0}, Variances{0.1}));
}

TEST(VariableTest, has_variances) {
  ASSERT_FALSE(makeVariable<double>(Values{double{}}).has_variances());
  ASSERT_FALSE(makeVariable<double>(Values{1.0}).has_variances());
  ASSERT_TRUE(
      makeVariable<double>(Values{1.0}, Variances{0.1}).has_variances());
  ASSERT_TRUE(makeVariable<double>(Dims(), Shape(), sc_units::m, Values{1.0},
                                   Variances{0.1})
                  .has_variances());
}

TEST(VariableTest, values_variances) {
  const auto var = makeVariable<double>(Values{1.0}, Variances{0.1});
  ASSERT_NO_THROW(var.values<double>());
  ASSERT_NO_THROW(var.variances<double>());
  ASSERT_TRUE(equals(var.values<double>(), {1.0}));
  ASSERT_TRUE(equals(var.variances<double>(), {0.1}));
}

template <typename Var> void test_set_variances(Var &var) {
  const auto v = var * (2.0 * sc_units::one);
  var.setVariances(Variable(var));
  ASSERT_TRUE(equals(var.template variances<double>(), {1.0, 2.0, 3.0}));
  // Fail because `var` has variances (setVariances uses only the values)
  EXPECT_THROW(var.setVariances(var * (2.0 * sc_units::one)),
               except::VariancesError);
  var.setVariances(v);
  ASSERT_TRUE(equals(var.template variances<double>(), {2.0, 4.0, 6.0}));

  const auto bad_dims = v.rename_dims({{Dim::X, Dim::Y}});
  EXPECT_THROW(var.setVariances(bad_dims), except::DimensionError);

  Variable bad_unit = copy(v);
  bad_unit.setUnit(sc_units::s);
  EXPECT_THROW(var.setVariances(bad_unit), except::UnitError);

  EXPECT_THROW(var.setVariances(astype(v, dtype<float>)), except::TypeError);
}

TEST(VariableTest, set_variances) {
  Variable var = makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                      Values{1.0, 2.0, 3.0});
  test_set_variances(var);
}

TEST(VariableTest, set_variances_rejects_ints) {
  Variable var = makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{});
  EXPECT_THROW(var.setVariances(var), except::VariancesError);
}

TEST(VariableTest, set_variances_remove) {
  Variable var =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{}, Variances{});
  EXPECT_TRUE(var.has_variances());
  EXPECT_NO_THROW(var.setVariances(Variable()));
  EXPECT_FALSE(var.has_variances());
}

TEST(VariableTest, set_variances_remove_int) {
  Variable var = makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{});
  EXPECT_FALSE(var.has_variances());
  EXPECT_NO_THROW(var.setVariances(Variable()));
  EXPECT_FALSE(var.has_variances());
}

TEST(VariableViewTest, set_variances) {
  Variable var = makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                      Values{1.0, 2.0, 3.0});
  auto view(var);
  test_set_variances(view);
  EXPECT_THROW(var.slice({Dim::X, 0}).setVariances(var.slice({Dim::X, 0})),
               except::VariancesError);
}

TEST(VariableViewTest, set_variances_slice_fail) {
  Variable var = makeVariable<double>(Dims{Dim::X}, Shape{3});
  EXPECT_THROW(var.slice({Dim::X, 0}).setVariances(var.slice({Dim::X, 0})),
               except::VariancesError);
}

TEST(VariableViewTest, create_with_variance) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                        Values{1.0, 2.0}, Variances{0.1, 0.2});
  ASSERT_NO_THROW_DISCARD(var.slice({Dim::X, 1, 2}));
  const auto slice = var.slice({Dim::X, 1, 2});
  ASSERT_TRUE(slice.has_variances());
  ASSERT_EQ(slice.variances<double>().size(), 1);
  ASSERT_EQ(slice.variances<double>()[0], 0.2);
  const auto reference =
      makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{2.0}, Variances{0.2});
  ASSERT_EQ(slice, reference);
}

TEST(VariableTest, variances_unsupported_type_fail) {
  ASSERT_NO_THROW(
      makeVariable<std::string>(Dims{Dim::X}, Shape{1}, Values{"a"}));
  ASSERT_THROW(makeVariable<std::string>(Dims{Dim::X}, Shape{1}, Values{"a"},
                                         Variances{"variances"}),
               except::VariancesError);
}

TEST(VariableTest, construct_view_dims) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2l, 3l});
  Variable vv(var.slice({Dim::X, 0, 2}));
  ASSERT_NO_THROW(Variable(var.slice({Dim::X, 0, 2}), Dimensions(Dim::Y, 2)));
}

TEST(VariableTest, construct_mult_dev_unit) {
  Variable refDiv = makeVariable<float>(
      Dims(), Shape(), sc_units::one / sc_units::m, Values{1.0f});
  Variable refMult =
      makeVariable<int32_t>(Dims(), Shape(), sc_units::kg, Values{1});
  EXPECT_EQ(1.0f / sc_units::m, refDiv);
  EXPECT_EQ(int32_t(1) * sc_units::kg, refMult);
}

TEST(VariableTest, datetime_dtype) {
  auto dt =
      makeVariable<scipp::core::time_point>(Values{scipp::core::time_point{}});
  EXPECT_EQ(dt.dtype(), dtype<scipp::core::time_point>);
}

TEST(VariableTest, construct_time_unit) {
  Variable refMult =
      makeVariable<int64_t>(Dims(), Shape(), sc_units::ns, Values{1000});
  EXPECT_EQ(int64_t(1000) * sc_units::ns, refMult);
}

TEST(VariableTest, array_params) {
  const auto parent =
      makeVariable<double>(Dims{Dim::X, Dim::Y, Dim::Z}, Shape{4, 2, 3});

  const Strides yz{3, 1};
  const Strides xz{3 * 2, 1};
  const Strides xy{3 * 2, 3};
  EXPECT_EQ(parent.array_params().strides(), Strides{parent.strides()});
  EXPECT_EQ(parent.slice({Dim::X, 1}).array_params().strides(), yz);
  EXPECT_EQ(parent.slice({Dim::Y, 1}).array_params().strides(), xz);
  EXPECT_EQ(parent.slice({Dim::Z, 1}).array_params().strides(), xy);

  const auto empty_1d = makeVariable<double>(Dims{Dim::X}, Shape{0});
  EXPECT_EQ(empty_1d.array_params().strides(), Strides{empty_1d.strides()});
  const auto empty_2d = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 0});
  EXPECT_EQ(empty_2d.array_params().strides(), Strides{empty_2d.strides()});
}

TEST(Variable, nested_Variable_copy) {
  const auto one = makeVariable<double>(Values{1.0});
  const auto two = makeVariable<double>(Values{2.0});
  const auto inner = copy(one);
  const auto outer = makeVariable<Variable>(Values{inner});
  auto copied = copy(outer);
  copied.value<Variable>() += one;
  EXPECT_NE(two, one);
  EXPECT_EQ(inner, one);
  EXPECT_EQ(outer.value<Variable>(), one);
  EXPECT_EQ(copied.value<Variable>(), two);
}

TEST(Variable, self_nesting_scalar_copy) {
  Variable inner = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 3});

  // 1 level of nesting
  Variable v1 = makeVariable<Variable>(Shape{}, Values{copy(inner)});
  ASSERT_NO_THROW_DISCARD(v1 = v1);
  ASSERT_EQ(v1.value<Variable>(), inner);
  ASSERT_THROW_DISCARD(v1.value<Variable>() = v1, std::invalid_argument);
  ASSERT_EQ(v1.value<Variable>(), inner);

  // 2 levels of nesting
  Variable v2 = makeVariable<Variable>(Shape{}, Values{v1});
  ASSERT_THROW_DISCARD(v1.value<Variable>() = v2, std::invalid_argument);

  // Works, replace content of v1 => not self nested.
  ASSERT_NO_THROW_DISCARD(v1 = v2);
  ASSERT_EQ(v1.value<Variable>().value<Variable>(), inner);
}

TEST(Variable, self_nesting_scalar_move) {
  Variable inner = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 3});

  // 2 levels of nesting
  Variable v1 = makeVariable<Variable>(Shape{}, Values{inner});
  Variable v2 = makeVariable<Variable>(Shape{}, Values{v1});
  ASSERT_THROW_DISCARD(v1.value<Variable>() = std::move(v2),
                       std::invalid_argument);
  v2 = makeVariable<Variable>(Shape{}, Values{v1});

  // Works, replace content of v1 => not self nested.
  ASSERT_NO_THROW_DISCARD(v1 = std::move(v2));
  ASSERT_EQ(v1.value<Variable>().value<Variable>(), inner);
}

TEST(Variable, self_nesting_array) {
  Variable inner1 = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 3});
  Variable inner2 =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{4, 5, 6});

  // 1 level of nesting
  Variable v1 = makeVariable<Variable>(Dims{Dim::Z}, Shape{2},
                                       Values{copy(inner1), copy(inner2)});
  ASSERT_NO_THROW_DISCARD(v1 = v1);
  ASSERT_EQ(v1.values<Variable>().front(), inner1);
  ASSERT_THROW_DISCARD(v1.values<Variable>().front() = v1,
                       std::invalid_argument);
  ASSERT_EQ(v1.values<Variable>().front(), inner1);
  for (auto &v : v1.values<Variable>()) {
    ASSERT_THROW_DISCARD(v = v1, std::invalid_argument);
  }
  ASSERT_EQ(v1.values<Variable>()[0], inner1);
  ASSERT_EQ(v1.values<Variable>()[1], inner2);

  // 2 levels of nesting
  Variable v2 =
      makeVariable<Variable>(Dims{Dim::Row}, Shape{2}, Values{v1, inner2});
  ASSERT_THROW_DISCARD(v1.values<Variable>()[0] = v2, std::invalid_argument);
  ASSERT_THROW_DISCARD(v1.values<Variable>()[1] = v2, std::invalid_argument);
  ASSERT_THROW_DISCARD(v2.values<Variable>()[0].values<Variable>()[0] = v2,
                       std::invalid_argument);
  ASSERT_THROW_DISCARD(v1.values<Variable>()[0] = v2.values<Variable>()[0],
                       std::invalid_argument);
  ASSERT_NO_THROW_DISCARD(v1.values<Variable>()[0] = v2.values<Variable>()[1]);
  ASSERT_EQ(v1.values<Variable>()[0], inner2);

  // Works, replace content of v1 => not self nested.
  ASSERT_NO_THROW_DISCARD(v1 = v2);
  ASSERT_EQ(v1.values<Variable>()[0].values<Variable>()[0], inner2);
  ASSERT_EQ(v1.values<Variable>()[0].values<Variable>()[1], inner2);
  ASSERT_EQ(v1.values<Variable>()[1], inner2);
}
