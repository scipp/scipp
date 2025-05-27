// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"
#include "test_print_variable.h"

#include "scipp/core/element/arg_list.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"

#include "transform_test_helpers.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;
using namespace scipp::testing;

namespace {
const char *name = "transform_test";
} // namespace

class TransformUnaryTest : public ::testing::Test {
protected:
  static constexpr auto op_in_place{
      overloaded{[](auto &x) { x *= 2.0; }, [](sc_units::Unit &) {}}};
  static constexpr auto op{
      overloaded{[](const auto x) { return x * 2.0; },
                 [](const sc_units::Unit &unit) { return unit; }}};

  template <typename T>
  static auto op_manual_values(const ElementArrayView<T> &values) {
    std::vector<double> res;
    res.reserve(values.size());
    std::transform(values.begin(), values.end(), std::back_inserter(res), op);
    return res;
  }

  template <typename T>
  static auto op_manual_variances(const ElementArrayView<T> &values,
                                  const ElementArrayView<T> &variances) {
    std::vector<double> res;
    res.reserve(values.size());
    std::transform(
        values.begin(), values.end(), variances.begin(),
        std::back_inserter(res), [](auto value, auto variance) {
          return op(ValueAndVariance<double>{value, variance}).variance;
        });
    return res;
  }
};

class TransformUnaryDenseTest
    : public TransformUnaryTest,
      public ::testing::WithParamInterface<std::tuple<Shape, bool>> {
protected:
  const Variable input_var;

  TransformUnaryDenseTest() : input_var{make_input_variable()} {}

  static Variable make_input_variable() {
    const auto &[shape, variances] = GetParam();
    return make_dense_variable<double>(shape, variances);
  }

  /// Note that this function modifies its input!
  // This is needed because we cannot make a copy for the input of
  // transform_in_place as that would result in a dense memory layout
  // which would prevent testing slicing / transposition.
  static void check_transform(Variable &var) {
    const auto result_return = transform<double>(var, op, name);
    EXPECT_TRUE(equals(result_return.values<double>(),
                       op_manual_values(var.values<double>())));
    if (var.has_variances()) {
      EXPECT_TRUE(equals(
          result_return.variances<double>(),
          op_manual_variances(var.values<double>(), var.variances<double>())));
    }

    transform_in_place<double>(var, op_in_place, name);
    // Non-in-place transform used to check result of in-place transform.
    EXPECT_EQ(var, result_return);
  }
};

INSTANTIATE_TEST_SUITE_P(Array, TransformUnaryDenseTest,
                         ::testing::Combine(::testing::ValuesIn(shapes()),
                                            ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(Scalar, TransformUnaryDenseTest,
                         ::testing::Combine(::testing::Values(Shape{0}),
                                            ::testing::Bool()));

TEST_P(TransformUnaryDenseTest, dense) {
  auto a = copy(input_var);
  check_transform(a);
}

TEST_P(TransformUnaryDenseTest, slice) {
  for (const auto &slices : scipp::testing::make_slice_combinations(
           input_var.dims().shape(), {Dim::X, Dim::Y, Dim::Z})) {
    auto a = slice(copy(input_var), slices);
    check_transform(a);
  }
}

TEST_P(TransformUnaryDenseTest, transpose) {
  const auto initial = transpose(input_var);

  const auto result_return = transform<double>(initial, op, name);
  auto result_in_place = copy(initial);
  transform_in_place<double>(result_in_place, op_in_place, name);

  const auto expected = transpose(transform<double>(input_var, op, name));
  EXPECT_EQ(result_return, expected);
  EXPECT_EQ(result_return, result_in_place);
}

namespace {
template <class Op>
Variable compute_on_buffer(const Variable &var,
                           const scipp::index bin_dim_index, const Op &op) {
  const auto bin_dim_label =
      var.bin_buffer<Variable>().dims().label(bin_dim_index);
  return make_bins(var.bin_indices(), bin_dim_label,
                   transform<double>(var.bin_buffer<Variable>(), op, name));
}
} // namespace

class TransformUnaryRegularBinsTest
    : public TransformUnaryTest,
      public ::testing::WithParamInterface<std::tuple<Shape, // event shape
                                                      Shape, // bin shape
                                                      scipp::index, // bin dim
                                                      bool          // variances
                                                      >> {
protected:
  Variable binned;

  TransformUnaryRegularBinsTest() : binned{make_input_variable()} {}

  static Variable make_input_variable() {
    const auto &[event_shape, bin_shape, bin_dim, variances] = GetParam();
    return make_binned_variable<double>(event_shape, bin_shape, bin_dim,
                                        variances);
  }

  static Variable compute_on_buffer(const Variable &var) {
    return ::compute_on_buffer(var, std::get<2>(GetParam()), op);
  }
};

INSTANTIATE_TEST_SUITE_P(OneD, TransformUnaryRegularBinsTest,
                         ::testing::Combine(::testing::ValuesIn(shapes(1)),
                                            ::testing::ValuesIn(shapes()),
                                            ::testing::Range(scipp::index{0},
                                                             scipp::index{1}),
                                            ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(TwoD, TransformUnaryRegularBinsTest,
                         ::testing::Combine(::testing::ValuesIn(shapes(2)),
                                            ::testing::ValuesIn(shapes()),
                                            ::testing::Range(scipp::index{0},
                                                             scipp::index{2}),
                                            ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(ThreeD, TransformUnaryRegularBinsTest,
                         ::testing::Combine(::testing::ValuesIn(shapes(3)),
                                            ::testing::ValuesIn(shapes()),
                                            ::testing::Range(scipp::index{0},
                                                             scipp::index{3}),
                                            ::testing::Bool()));

TEST_P(TransformUnaryRegularBinsTest, full) {
  const auto result_return = transform<double>(binned, op, name);
  EXPECT_EQ(result_return, compute_on_buffer(binned));

  transform_in_place<double>(binned, op_in_place, name);
  EXPECT_EQ(binned, result_return);
}

TEST_P(TransformUnaryRegularBinsTest, slices_in_bin) {
  for (const auto &slices : scipp::testing::make_slice_combinations(
           binned.dims().shape(), {Dim{"i0"}, Dim{"i1"}, Dim{"i2"}})) {
    auto full_view = make_input_variable();
    auto sliced = slice(full_view, slices);

    const auto result_return = transform<double>(sliced, op, name);
    EXPECT_EQ(result_return, compute_on_buffer(sliced));

    transform_in_place<double>(sliced, op_in_place, name);
    EXPECT_EQ(sliced, result_return);
  }
}

class TransformUnaryIrregularBinsTest
    : public TransformUnaryTest,
      public ::testing::WithParamInterface<std::tuple<Variable, bool>> {
protected:
  const Variable indices;
  const Variable input_buffer;
  const Variable input;

  TransformUnaryIrregularBinsTest()
      : indices{std::get<Variable>(GetParam())},
        input_buffer{make_dense_variable<double>(Shape{index_volume(indices)},
                                                 std::get<bool>(GetParam()))},
        input{make_bins(indices, Dim::X, input_buffer)} {}
};

INSTANTIATE_TEST_SUITE_P(
    OneDIndices, TransformUnaryIrregularBinsTest,
    ::testing::Combine(::testing::ValuesIn(irregular_bin_indices_1d()),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    TwoDIndices, TransformUnaryIrregularBinsTest,
    ::testing::Combine(::testing::ValuesIn(irregular_bin_indices_2d()),
                       ::testing::Bool()));

TEST_P(TransformUnaryIrregularBinsTest, elements_of_bins) {
  const auto result = transform<double>(input, op, name);
  const auto expected =
      make_bins(indices, Dim::X, transform<double>(input_buffer, op, name));
  EXPECT_EQ(result, expected);
  auto result_in_place = copy(input);
  transform_in_place<double>(result_in_place, op_in_place, name);
  EXPECT_EQ(result_in_place, expected);
}

TEST(TransformUnaryTest, in_place_unit_change) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                        Values{1.0, 2.0});
  const auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, sc_units::Unit(sc_units::m * sc_units::m),
      Values{1.0, 4.0});
  auto op_ = [](auto &&a) { a *= a; };
  Variable result;

  result = copy(var);
  transform_in_place<double>(result, op_, name);
  EXPECT_EQ(result, expected);

  // Unit changes but we are transforming only parts of data -> not possible.
  result = copy(var);
  EXPECT_THROW(transform_in_place<double>(result.slice({Dim::X, 1}), op_, name),
               except::UnitError);
}

TEST(TransformUnaryTest, drop_variances_when_not_supported_on_out_type) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2},
                                  Variances{1.1, 2.2});
  const auto result = transform<double>(
      var,
      overloaded{[](const sc_units::Unit &) { return sc_units::none; },
                 [](const auto) { return true; }},
      name);
  EXPECT_EQ(result,
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, true}));
}

TEST(TransformUnaryTest, apply_implicit_conversion) {
  const auto var =
      makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  // The functor returns double, so the output type is also double.
  auto out = transform<float>(
      var,
      overloaded{[](const auto x) { return -1.0 * x; },
                 [](const sc_units::Unit &unit) { return unit; }},
      name);
  EXPECT_TRUE(equals(out.values<double>(), {-1.1f, -2.2f}));
}

TEST(TransformUnaryTest, apply_dtype_preserved) {
  const auto varD =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  const auto varF =
      makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  auto outD =
      transform<double, float>(varD, [](const auto x) { return -x; }, name);
  auto outF =
      transform<double, float>(varF, [](const auto x) { return -x; }, name);
  EXPECT_TRUE(equals(outD.values<double>(), {-1.1, -2.2}));
  EXPECT_TRUE(equals(outF.values<float>(), {-1.1f, -2.2f}));
}

TEST(TransformUnaryTest, dtype_bool) {
  auto var = makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false});

  EXPECT_EQ(
      transform<bool>(var,
                      overloaded{[](const sc_units::Unit &u) { return u; },
                                 [](const auto x) { return !x; }},
                      name),
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true}));
}
