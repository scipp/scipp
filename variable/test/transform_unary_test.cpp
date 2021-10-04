// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

namespace {
const char *name = "transform_test";

const std::vector<Shape> shapes{Shape{1},       Shape{2},       Shape{3},
                                Shape{5},       Shape{16},      Shape{1, 1},
                                Shape{1, 2},    Shape{3, 1},    Shape{2, 8},
                                Shape{5, 7},    Shape{1, 1, 1}, Shape{1, 1, 4},
                                Shape{1, 5, 1}, Shape{7, 1, 1}, Shape{2, 8, 4}};

auto make_slices(const scipp::span<const scipp::index> &shape) {
  std::vector<Slice> res;
  const std::vector dim_labels{Dim::X, Dim::Y, Dim::Z};
  for (size_t dim = 0; dim < 3; ++dim) {
    if (shape.size() > dim && shape[dim] > 1) {
      res.emplace_back(dim_labels.at(dim), 0, shape[dim] - 1);
      res.emplace_back(dim_labels.at(dim), 0, shape[dim] / 2);
      res.emplace_back(dim_labels.at(dim), 2, shape[dim]);
    }
  }
  return res;
}

auto make_dim_labels(const scipp::index ndim,
                     std::initializer_list<Dim> choices) {
  assert(ndim <= scipp::size(choices));
  Dims result(choices);
  result.data.resize(ndim);
  return result;
}

auto volume(const Shape &shape) {
  return std::accumulate(shape.data.begin(), shape.data.end(), 1,
                         std::multiplies<scipp::index>{});
}

template <class T>
static Variable make_dense_variable(const Shape &shape, const bool variances) {
  const auto ndim = scipp::size(shape.data);
  const auto dims = make_dim_labels(ndim, {Dim::X, Dim::Y, Dim::Z});
  auto var = variances ? makeVariable<T>(dims, shape, Values{}, Variances{})
                       : makeVariable<T>(dims, shape, Values{});

  const auto total_size = var.dims().volume();
  std::iota(var.template values<T>().begin(), var.template values<T>().end(),
            -total_size / 2.0);
  if (variances) {
    std::generate(var.template variances<T>().begin(),
                  var.template variances<T>().end(),
                  [x = -total_size / 20.0, total_size]() mutable {
                    return x += 10.0 / total_size;
                  });
  }
  return var;
}

auto make_regular_bin_indices(const scipp::index size, const Shape &shape,
                              const scipp::index ndim) {
  const auto n_bins = volume(shape);
  std::vector<index_pair> aux(n_bins);
  std::generate(begin(aux), end(aux),
                [lower = scipp::index{0}, d_size = static_cast<double>(size),
                 d_n_bins = static_cast<double>(n_bins)]() mutable {
                  const auto upper = static_cast<scipp::index>(
                      static_cast<double>(lower) + d_size / d_n_bins);
                  const index_pair res{lower, upper};
                  lower = upper;
                  return res;
                });
  aux.back().second = size;
  return makeVariable<index_pair>(
      make_dim_labels(ndim, {Dim{"i0"}, Dim{"i1"}, Dim{"i2"}}), shape,
      Values(aux));
}
} // namespace

class BaseTransformUnaryTest : public ::testing::Test {
protected:
  static constexpr auto op_in_place{
      overloaded{[](auto &x) { x *= 2.0; }, [](units::Unit &) {}}};
  static constexpr auto op{
      overloaded{[](const auto x) { return x * 2.0; },
                 [](const units::Unit &unit) { return unit; }}};

  template <typename T>
  static auto op_manual_values(const ElementArrayView<T> values) {
    std::vector<double> res;
    res.reserve(values.size());
    std::transform(values.begin(), values.end(), std::back_inserter(res), op);
    return res;
  }

  template <typename T>
  static auto op_manual_variances(const ElementArrayView<T> values,
                                  const ElementArrayView<T> variances) {
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

class TransformUnaryTest
    : public BaseTransformUnaryTest,
      public ::testing::WithParamInterface<std::tuple<Shape, bool>> {
protected:
  const Variable input_var;

  TransformUnaryTest() : input_var{make_input_variable()} {}

  static Variable make_input_variable() {
    const auto &[shape, variances] = GetParam();
    return make_dense_variable<double>(shape, variances);
  }
};

INSTANTIATE_TEST_SUITE_P(Array, TransformUnaryTest,
                         ::testing::Combine(::testing::ValuesIn(shapes),
                                            ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(Scalar, TransformUnaryTest,
                         ::testing::Combine(::testing::Values(Shape{0}),
                                            ::testing::Bool()));

TEST_P(TransformUnaryTest, dense) {
  const auto result_return = transform<double>(input_var, op, name);
  Variable result_in_place = copy(input_var);
  transform_in_place<double>(result_in_place, op_in_place, name);

  EXPECT_TRUE(equals(result_in_place.values<double>(),
                     op_manual_values(input_var.values<double>())));
  if (input_var.hasVariances()) {
    EXPECT_TRUE(equals(result_in_place.variances<double>(),
                       op_manual_variances(input_var.values<double>(),
                                           input_var.variances<double>())));
  }
  // In-place transform used to check result of non-in-place transform.
  EXPECT_EQ(result_return, result_in_place);
}

TEST_P(TransformUnaryTest, slice) {
  for (const Slice &slice : make_slices(input_var.dims().shape())) {
    const auto initial = input_var.slice(slice);

    const auto result_return = transform<double>(initial, op, name);
    Variable result_in_place_buffer = copy(input_var);
    auto result_in_place = result_in_place_buffer.slice(slice);
    transform_in_place<double>(result_in_place, op_in_place, name);

    EXPECT_TRUE(equals(result_return.values<double>(),
                       op_manual_values(initial.values<double>())));
    if (initial.hasVariances()) {
      EXPECT_TRUE(equals(result_return.variances<double>(),
                         op_manual_variances(initial.values<double>(),
                                             initial.variances<double>())));
    }
    // In-place transform used to check result of non-in-place transform.
    EXPECT_EQ(result_return, result_in_place);
  }
}

TEST_P(TransformUnaryTest, transpose) {
  const auto initial = transpose(input_var);

  const auto result_return = transform<double>(initial, op, name);
  auto result_in_place = copy(initial);
  transform_in_place<double>(result_in_place, op_in_place, name);

  const auto expected = transpose(transform<double>(input_var, op, name));
  EXPECT_EQ(result_return, expected);
  EXPECT_EQ(result_return, result_in_place);
}

TEST_P(TransformUnaryTest, elements_of_bins) {
  const auto &[base_event_shape, variances] = GetParam();
  for (const auto &bin_shape : shapes) {
    const auto n_bin = volume(bin_shape);
    for (scipp::index bin_dim = 0; bin_dim < scipp::size(base_event_shape.data);
         ++bin_dim) {
      auto event_shape = base_event_shape;
      event_shape.data.at(bin_dim) *= n_bin;

      const auto buffer = make_dense_variable<double>(event_shape, variances);
      const auto bin_dim_label = buffer.dims().labels()[bin_dim];
      const auto indices = make_regular_bin_indices(
          event_shape.data.at(bin_dim), bin_shape, scipp::size(bin_shape.data));
      auto var = make_bins(indices, bin_dim_label, copy(buffer));

      const auto result = transform<double>(var, op, name);
      transform_in_place<double>(var, op_in_place, name);

      const auto expected = make_bins(indices, bin_dim_label,
                                      transform<double>(buffer, op, name));
      EXPECT_EQ(result, expected);
      EXPECT_EQ(result, var);
    }
  }
}

class TransformUnaryIrregularBinsTest
    : public BaseTransformUnaryTest,
      public ::testing::WithParamInterface<std::tuple<Variable, bool>> {
protected:
  static constexpr auto op_in_place{
      overloaded{[](auto &x) { x *= 2.0; }, [](units::Unit &) {}}};
  static constexpr auto op{
      overloaded{[](const auto x) { return x * 2.0; },
                 [](const units::Unit &unit) { return unit; }}};

  const Variable indices;
  const Variable input_buffer;
  const Variable input;

  TransformUnaryIrregularBinsTest()
      : indices{std::get<Variable>(GetParam())},
        input_buffer{make_dense_variable<double>(Shape{index_volume(indices)},
                                                 std::get<bool>(GetParam()))},
        input{make_bins(indices, Dim::X, input_buffer)} {}

  static scipp::index index_volume(const Variable &indices) {
    const auto &values = indices.values<index_pair>();
    if (values.begin() == values.end())
      return scipp::index{0};

    const auto min = std::min_element(values.begin(), values.end(),
                                      [](const auto a, const auto b) {
                                        return a.first < b.first;
                                      })
                         ->first;
    const auto max = std::max_element(values.begin(), values.end(),
                                      [](const auto a, const auto b) {
                                        return a.second < b.second;
                                      })
                         ->second;
    return max - min;
  }
};

INSTANTIATE_TEST_SUITE_P(
    OneDIndices, TransformUnaryIrregularBinsTest,
    ::testing::Combine(
        ::testing::Values(
            makeVariable<index_pair>(Dims{Dim{"i0"}}, Shape{0}, Values{}),
            makeVariable<index_pair>(Dims{Dim{"i0"}}, Shape{2},
                                     Values{index_pair{0, 2},
                                            index_pair{2, 3}}),
            makeVariable<index_pair>(Dims{Dim{"i0"}}, Shape{2},
                                     Values{index_pair{0, 0},
                                            index_pair{0, 3}}),
            makeVariable<index_pair>(Dims{Dim{"i0"}}, Shape{2},
                                     Values{index_pair{0, 4},
                                            index_pair{4, 4}}),
            makeVariable<index_pair>(Dims{Dim{"i0"}}, Shape{3},
                                     Values{index_pair{0, 2}, index_pair{2, 2},
                                            index_pair{2, 3}}),
            makeVariable<index_pair>(Dims{Dim{"i0"}}, Shape{3},
                                     Values{index_pair{0, 1}, index_pair{1, 3},
                                            index_pair{3, 5}})),
        ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    TwoDIndices, TransformUnaryIrregularBinsTest,
    ::testing::Combine(
        ::testing::Values(
            makeVariable<index_pair>(Dims{Dim{"i0"}, Dim{"i1"}}, Shape{2, 2},
                                     Values{index_pair{0, 2}, index_pair{2, 3},
                                            index_pair{3, 5},
                                            index_pair{5, 6}}),
            makeVariable<index_pair>(Dims{Dim{"i0"}, Dim{"i1"}}, Shape{1, 2},
                                     Values{index_pair{0, 1},
                                            index_pair{1, 4}}),
            makeVariable<index_pair>(Dims{Dim{"i0"}, Dim{"i1"}}, Shape{2, 2},
                                     Values{index_pair{0, 1}, index_pair{2, 4},
                                            index_pair{4, 4},
                                            index_pair{6, 7}}),
            makeVariable<index_pair>(Dims{Dim{"i0"}, Dim{"i1"}}, Shape{0, 0},
                                     Values{})),
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
  const auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{1.0, 2.0});
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{2},
                           units::Unit(units::m * units::m), Values{1.0, 4.0});
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
  const auto result =
      transform<double>(var,
                        overloaded{[](const units::Unit &unit) { return unit; },
                                   [](const auto) { return true; }},
                        name);
  EXPECT_EQ(result,
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, true}));
}

TEST(TransformUnaryTest, apply_implicit_conversion) {
  const auto var =
      makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  // The functor returns double, so the output type is also double.
  auto out =
      transform<float>(var,
                       overloaded{[](const auto x) { return -1.0 * x; },
                                  [](const units::Unit &unit) { return unit; }},
                       name);
  EXPECT_TRUE(equals(out.values<double>(), {-1.1f, -2.2f}));
}

TEST(TransformUnaryTest, apply_dtype_preserved) {
  const auto varD =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  const auto varF =
      makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  auto outD = transform<double, float>(
      varD, [](const auto x) { return -x; }, name);
  auto outF = transform<double, float>(
      varF, [](const auto x) { return -x; }, name);
  EXPECT_TRUE(equals(outD.values<double>(), {-1.1, -2.2}));
  EXPECT_TRUE(equals(outF.values<float>(), {-1.1f, -2.2f}));
}

TEST(TransformUnaryTest, dtype_bool) {
  auto var = makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false});

  EXPECT_EQ(transform<bool>(var,
                            overloaded{[](const units::Unit &u) { return u; },
                                       [](const auto x) { return !x; }},
                            name),
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true}));
}
