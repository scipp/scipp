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
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

#include "transform_test_helpers.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;
using namespace scipp::testing;

namespace {
const char *name = "transform_test";

const std::vector<std::vector<sc_units::Dim>> dim_combinations{
    {Dim::X},
    {Dim::Y},
    {Dim::Z},
    {Dim::X, Dim::Y},
    {Dim::X, Dim::Z},
    {Dim::Y, Dim::Z},
    {Dim::X, Dim::Y, Dim::Z}};

std::optional<Variable> slice_to_scalar(Variable var,
                                        std::span<const sc_units::Dim> dims) {
  for (const auto &dim : dims) {
    if (!var.dims().contains(dim) || var.dims().at(dim) == 0) {
      return std::nullopt;
    }
    var = var.slice(Slice{dim, 0, -1});
  }
  return var;
}
} // namespace

class TransformBinaryTest : public ::testing::Test {
protected:
  static constexpr auto op_in_place{[](auto &x, const auto &y) { x *= y; }};
  static constexpr auto op{[](const auto &x, const auto &y) { return x * y; }};

  template <class T, class U>
  static auto op_manual_values(const Variable &a, const Variable &b) {
    assert(a.dims() == b.dims());

    std::vector<decltype(std::declval<T>() * std::declval<U>())> res;
    res.reserve(a.dims().volume());
    const auto &a_values = a.values<T>();
    const auto &b_values = b.values<U>();
    std::transform(a_values.begin(), a_values.end(), b_values.begin(),
                   std::back_inserter(res), op);
    return res;
  }

  template <class T, class U>
  static auto op_manual_variances(const Variable &a, const Variable &b) {
    assert(a.dims() == b.dims());

    std::vector<decltype(std::declval<T>() * std::declval<U>())> res;
    res.reserve(a.dims().volume());
    const auto &a_values = a.values<T>();
    const auto &b_values = b.values<U>();
    const auto &a_variances = a.variances<T>();
    const auto &b_variances = b.variances<U>();
    for (auto aval = a_values.begin(), avar = a_variances.begin(),
              bval = b_values.begin(), bvar = b_variances.begin();
         aval != a_values.end(); ++aval, ++avar, ++bval, ++bvar) {
      res.push_back(op(ValueAndVariance<T>(*aval, *avar),
                       ValueAndVariance<U>(*bval, *bvar))
                        .variance);
    }
    return res;
  }
};

class TransformBinaryDenseTest
    : public TransformBinaryTest,
      public ::testing::WithParamInterface<std::tuple<Shape, bool>> {
protected:
  const Variable input1;
  const Variable input2;

  TransformBinaryDenseTest()
      : input1{make_input_variable(0, 1)}, input2{make_input_variable(10, 2)} {}

  static Variable make_input_variable(const double offset, const double scale) {
    const auto &[shape, variances] = GetParam();
    return make_dense_variable<double>(shape, variances, offset, scale);
  }

  /// Note that this function modifies its inputs!
  // This is needed because we cannot make a copy for the input of
  // transform_in_place as that would result in a dense memory layout
  // which would prevent testing slicing / transposition.
  static void check_transform_combinations(Variable &a, Variable &b) {
    const auto do_check = [](Variable &x, Variable &y) {
      if (!x.dims().includes(y.dims())) {
        // Cannot broadcast y to x, therefore cannot use manual op.
        return false;
      }

      const auto y_for_manual =
          y.dims() == x.dims() ? y : y.broadcast(x.dims());
      const auto xy = transform<pair_self_t<double>>(x, y, op, name);
      EXPECT_TRUE(equals(xy.values<double>(),
                         op_manual_values<double, double>(x, y_for_manual)));
      if (std::get<bool>(GetParam())) {
        EXPECT_TRUE(
            equals(xy.variances<double>(),
                   op_manual_variances<double, double>(x, y_for_manual)));
      }

      transform_in_place<pair_self_t<double>>(x, y, op_in_place, name);
      EXPECT_EQ(x, xy);
      return true;
    };

    // The assertion ensures that at least one combination of tests is run.
    ASSERT_TRUE(do_check(a, b) || do_check(b, a));
  }
};

// Subset of tests involving broadcast, where the broadcast operand should not
// have variances.
class TransformBinaryDenseBroadcastTest : public TransformBinaryDenseTest {};

INSTANTIATE_TEST_SUITE_P(Array, TransformBinaryDenseTest,
                         ::testing::Combine(::testing::ValuesIn(shapes()),
                                            ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(Scalar, TransformBinaryDenseTest,
                         ::testing::Combine(::testing::Values(Shape{0}),
                                            ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(Array, TransformBinaryDenseBroadcastTest,
                         ::testing::Combine(::testing::ValuesIn(shapes()),
                                            ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(Scalar, TransformBinaryDenseBroadcastTest,
                         ::testing::Combine(::testing::Values(Shape{0}),
                                            ::testing::Values(false)));

TEST_P(TransformBinaryDenseTest, matching_shapes) {
  auto a = copy(input1);
  auto b = copy(input2);
  check_transform_combinations(a, b);
}

TEST_P(TransformBinaryDenseBroadcastTest, scalar_and_array) {
  auto a = copy(input1);
  auto s = std::get<bool>(GetParam())
               ? makeVariable<double>(Values{2.1}, Variances{1.3})
               : makeVariable<double>(Values{2.1});
  check_transform_combinations(a, s);
}

TEST_P(TransformBinaryDenseTest, slices) {
  for (const auto &slices : scipp::testing::make_slice_combinations(
           input1.dims().shape(), {Dim::X, Dim::Y, Dim::Z})) {
    auto a = slice(copy(input1), slices);
    auto b = slice(copy(input2), slices);
    check_transform_combinations(a, b);
    // Make one input a full view of its data.
    auto dense_a = copy(a);
    check_transform_combinations(dense_a, b);
  }
}

TEST_P(TransformBinaryDenseBroadcastTest, broadcast) {
  for (const auto &dims : dim_combinations) {
    auto sliced_ = slice_to_scalar(input2, dims);
    if (!sliced_)
      continue;
    auto sliced = sliced_.value();

    auto a = copy(input1);
    check_transform_combinations(a, sliced);

    auto dense_b = copy(sliced);
    check_transform_combinations(a, dense_b);
  }
}

TEST_P(TransformBinaryDenseTest, transpose) {
  auto a = copy(input1);
  auto b = transpose(copy(transpose(input2)));
  check_transform_combinations(a, b);
}

TEST_P(TransformBinaryDenseTest, transposed_layout) {
  const auto b = copy(transpose(input2));

  const auto ab = transform<pair_self_t<double>>(input1, b, op, name);
  const auto ab_expected =
      transform<pair_self_t<double>>(input1, input2, op, name);
  EXPECT_EQ(ab, ab_expected);

  const auto ba = transform<pair_self_t<double>>(b, input1, op, name);
  const auto ba_expected =
      transpose(transform<pair_self_t<double>>(input2, input1, op, name),
                ba.dims().labels());
  EXPECT_EQ(ba, ba_expected);

  auto a_in_place = copy(input1);
  transform_in_place<double>(a_in_place, b, op_in_place, name);
  EXPECT_EQ(a_in_place, ab);

  auto b_in_place = copy(b);
  transform_in_place<double>(b_in_place, input1, op_in_place, name);
  EXPECT_EQ(b_in_place, ba);
}

TEST_F(TransformBinaryTest, dims_and_shape_fail_in_place) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2});
  auto b = makeVariable<double>(Dims{Dim::Y}, Shape{2});
  auto c = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2});

  EXPECT_ANY_THROW(
      transform_in_place<pair_self_t<double>>(a, b, op_in_place, name));
  EXPECT_ANY_THROW(
      transform_in_place<pair_self_t<double>>(a, c, op_in_place, name));
}

TEST_F(TransformBinaryTest, dims_and_shape_fail) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{4});
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{2});
  auto c = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2});

  EXPECT_ANY_THROW(auto v = transform<pair_self_t<double>>(a, b, op, name));
  EXPECT_ANY_THROW(auto v = transform<pair_self_t<double>>(a, c, op, name));
}

TEST_F(TransformBinaryTest, dense_mixed_type) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  const auto b = makeVariable<float>(Values{3.3});

  const auto ab =
      transform<pair_custom_t<std::tuple<double, float>>>(a, b, op, name);
  const auto ba =
      transform<pair_custom_t<std::tuple<float, double>>>(b, a, op, name);
  transform_in_place<pair_custom_t<std::tuple<double, float>>>(
      a, b, op_in_place, name);

  EXPECT_TRUE(equals(a.values<double>(), {1.1 * 3.3f, 2.2 * 3.3f}));
  EXPECT_EQ(ab, ba);
  EXPECT_EQ(ab, a);
  EXPECT_EQ(ba, a);
}

TEST_F(TransformBinaryTest, in_place_self_overlap_without_variance_1d) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  auto reference = a * a.slice({Dim::X, 1});
  transform_in_place<pair_self_t<double>>(a, a.slice({Dim::X, 1}), op_in_place,
                                          name);
  ASSERT_EQ(a, reference);
}

TEST_F(TransformBinaryTest, in_place_self_overlap_without_variance_2d) {
  auto original = makeVariable<double>(Dimensions{{Dim::X, 2}, {Dim::Y, 2}},
                                       Values{1, 2, 3, 4});
  auto reference = makeVariable<double>(Dimensions{{Dim::X, 2}, {Dim::Y, 2}},
                                        Values{1, 6, 6, 16});
  Variable relabeled =
      fold(flatten(original, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z), Dim::Z,
           Dimensions{{Dim::Y, 2}, {Dim::X, 2}});
  ASSERT_EQ(original.data_handle(), relabeled.data_handle());
  ASSERT_NE(original.dims(), relabeled.dims());
  transform_in_place<pair_self_t<double>>(original, relabeled, op_in_place,
                                          name);
  ASSERT_EQ(original, reference);
}

TEST_F(TransformBinaryTest, in_place_self_overlap_with_variance) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2},
                                Variances{1.0, 2.0});
  auto reference = a * copy(broadcast(a.slice({Dim::X, 1}), a.dims()));
  // With self-overlap the implementation needs to make a copy of the rhs. This
  // is a regression test: An initial implementation was unintentionally
  // dropping the variances when making that copy.
  transform_in_place<pair_self_t<double>>(
      a, a.slice({Dim::X, 1}),
      overloaded{op_in_place, transform_flags::force_variance_broadcast}, name);
  ASSERT_EQ(a, reference);
}

TEST_F(TransformBinaryTest, in_place_unit_change) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                        Values{1.0, 2.0});
  const auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, sc_units::Unit(sc_units::m * sc_units::m),
      Values{1.0, 4.0});
  auto op_ = [](auto &&a, auto &&b) { a *= b; };
  Variable result;

  result = var;
  transform_in_place<pair_self_t<double>>(result, var, op_, name);
  EXPECT_EQ(result, expected);

  // Unit changes but we are transforming only parts of data -> not possible.
  result = var;
  EXPECT_THROW(
      transform_in_place<pair_self_t<double>>(
          result.slice({Dim::X, 1}), var.slice({Dim::X, 1}), op_, name),
      except::UnitError);
}

TEST(TransformTest, binary_dtype_bool) {
  auto var = makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false});

  EXPECT_EQ(transform<pair_self_t<bool>>(
                var, var,
                overloaded{[](const sc_units::Unit &a, const sc_units::Unit &) {
                             return a;
                           },
                           [](const auto x, const auto y) { return !x || y; }},
                name),
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, true}));

  transform_in_place<bool>(
      var, overloaded{[](sc_units::Unit &) {}, [](auto &x) { x = !x; }}, name);
  EXPECT_EQ(var,
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true}));

  transform_in_place<pair_self_t<bool>>(
      var, var,
      overloaded{[](sc_units::Unit &, const sc_units::Unit &) {},
                 [](auto &x, const auto &y) { x = !x || y; }},
      name);
  EXPECT_EQ(var,
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, true}));
}

namespace {
template <class Op>
Variable compute_on_bin_buffer(const Variable &a, const Variable &b,
                               const scipp::index bin_dim_index, const Op &op) {
  const auto bin_dim_label =
      a.bin_buffer<Variable>().dims().label(bin_dim_index);
  return make_bins(a.bin_indices(), bin_dim_label,
                   transform<double>(a.bin_buffer<Variable>(),
                                     b.bin_buffer<Variable>(), op, name));
}

Variable element_as_scalar(const Variable &var, const scipp::index i) {
  const auto val = var.values<double>()[i];
  if (var.has_variances())
    return makeVariable<double>(Shape{}, Values{val},
                                Variances{var.variances<double>()[i]});
  return makeVariable<double>(Shape{}, Values{val});
}

template <class Op, class OpInPlace>
void check_binned_with_dense(Variable &binned, const Variable &dense,
                             const scipp::index bin_dim_index, const Op &op,
                             const OpInPlace &op_in_place) {
  const auto indices = binned.bin_indices();
  const auto &buffer = binned.bin_buffer<Variable>();

  const auto binned_dense = transform<double>(binned, dense, op, name);
  const auto dense_binned = transform<double>(dense, binned, op, name);

  for (scipp::index i = 0; i < indices.dims().volume(); ++i) {
    const auto values = indices.values<index_pair>();
    const auto &[begin, end] = values[i];
    const auto bin_dim = buffer.dims().label(bin_dim_index);
    const auto bin = buffer.slice(Slice{bin_dim, begin, end});
    const auto dense_slice = element_as_scalar(dense, i);
    EXPECT_EQ(binned_dense.template values<bucket<Variable>>()[i],
              transform<double>(bin, dense_slice, op, name));
    EXPECT_EQ(dense_binned.template values<bucket<Variable>>()[i],
              transform<double>(dense_slice, bin, op, name));
  }

  transform_in_place<double>(binned, dense, op_in_place, name);
  EXPECT_EQ(binned, binned_dense);
}
} // namespace

class TransformBinaryRegularBinsTest
    : public TransformBinaryTest,
      public ::testing::WithParamInterface<std::tuple<Shape, // event shape
                                                      Shape, // bin shape
                                                      scipp::index, // bin dim
                                                      bool          // variances
                                                      >> {
protected:
  Variable binned1;
  Variable binned2;

  TransformBinaryRegularBinsTest()
      : binned1{make_input_variable(0, 1)},
        binned2{make_input_variable(3, 10)} {}

  static Variable make_input_variable(const double offset, const double scale) {
    const auto &[event_shape, bin_shape, bin_dim, variances] = GetParam();
    return make_binned_variable<double>(event_shape, bin_shape, bin_dim,
                                        variances, offset, scale);
  }

  static Variable compute_on_buffer(const Variable &a, const Variable &b) {
    return compute_on_bin_buffer(a, b, std::get<2>(GetParam()), op);
  }

  static Variable make_dense_bin_dims() {
    auto var = make_dense_variable<double>(std::get<1>(GetParam()),
                                           std::get<3>(GetParam()), 2.1, 3.2);
    var = var.rename_dims({{Dim::X, Dim{"i0"}}});
    if (var.dims().ndim() > 1)
      var = var.rename_dims({{Dim::Y, Dim{"i1"}}});
    if (var.dims().ndim() > 2)
      var = var.rename_dims({{Dim::Z, Dim{"i2"}}});
    return var;
  }
};

// Subset of tests involving broadcast, where the broadcast operand should not
// have variances.
class TransformBinaryRegularBinsBroadcastTest
    : public TransformBinaryRegularBinsTest {};

INSTANTIATE_TEST_SUITE_P(OneD, TransformBinaryRegularBinsTest,
                         ::testing::Combine(::testing::ValuesIn(shapes(1)),
                                            ::testing::ValuesIn(shapes()),
                                            ::testing::Range(scipp::index{0},
                                                             scipp::index{1}),
                                            ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(TwoD, TransformBinaryRegularBinsTest,
                         ::testing::Combine(::testing::ValuesIn(shapes(2)),
                                            ::testing::ValuesIn(shapes()),
                                            ::testing::Range(scipp::index{0},
                                                             scipp::index{2}),
                                            ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(ThreeD, TransformBinaryRegularBinsTest,
                         ::testing::Combine(::testing::ValuesIn(shapes(3)),
                                            ::testing::ValuesIn(shapes()),
                                            ::testing::Range(scipp::index{0},
                                                             scipp::index{3}),
                                            ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(OneD, TransformBinaryRegularBinsBroadcastTest,
                         ::testing::Combine(::testing::ValuesIn(shapes(1)),
                                            ::testing::ValuesIn(shapes()),
                                            ::testing::Range(scipp::index{0},
                                                             scipp::index{1}),
                                            ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(TwoD, TransformBinaryRegularBinsBroadcastTest,
                         ::testing::Combine(::testing::ValuesIn(shapes(2)),
                                            ::testing::ValuesIn(shapes()),
                                            ::testing::Range(scipp::index{0},
                                                             scipp::index{2}),
                                            ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(ThreeD, TransformBinaryRegularBinsBroadcastTest,
                         ::testing::Combine(::testing::ValuesIn(shapes(3)),
                                            ::testing::ValuesIn(shapes()),
                                            ::testing::Range(scipp::index{0},
                                                             scipp::index{3}),
                                            ::testing::Values(false)));

TEST_P(TransformBinaryRegularBinsTest, binned_with_binned) {
  const auto ab = transform<double>(binned1, binned2, op, name);
  EXPECT_EQ(ab, compute_on_buffer(binned1, binned2));

  transform_in_place<double>(binned1, binned2, op_in_place, name);
  EXPECT_EQ(binned1, ab);
}

TEST_P(TransformBinaryRegularBinsBroadcastTest, binned_with_binned_broadcast) {
  for (const auto &dims : dim_combinations) {
    auto sliced_ = slice_to_scalar(binned2, dims);
    if (!sliced_)
      continue;
    const auto sliced = sliced_.value();
    auto a = copy(binned1);
    const auto expected =
        transform<double>(a, sliced.broadcast(a.dims()), op, name);

    for (auto &b : {sliced, copy(sliced)}) {
      EXPECT_EQ(transform<double>(a, b, op, name), expected);
      transform_in_place<double>(a, b, op_in_place, name);
      EXPECT_EQ(a, expected);
      auto mutable_b = b;
      EXPECT_THROW(transform_in_place<double>(mutable_b, a, op_in_place, name),
                   except::DimensionError);
    }
  }
}

TEST_P(TransformBinaryRegularBinsBroadcastTest, binned_with_dense) {
  check_binned_with_dense(binned1, make_dense_bin_dims(),
                          std::get<2>(GetParam()), op, op_in_place);
}

class TransformBinaryIrregularBinsTest
    : public TransformBinaryTest,
      public ::testing::WithParamInterface<std::tuple<Variable, bool>> {
protected:
  const Variable indices;
  const Variable buffer1;
  const Variable buffer2;
  Variable binned1;
  Variable binned2;

  TransformBinaryIrregularBinsTest()
      : indices{std::get<Variable>(GetParam())},
        buffer1{make_dense_variable<double>(Shape{index_volume(indices)},
                                            std::get<bool>(GetParam()), 0, 1)},
        buffer2{make_dense_variable<double>(
            Shape{index_volume(indices)}, std::get<bool>(GetParam()), 3.1, 10)},
        binned1{make_bins(indices, Dim::X, buffer1)},
        binned2{make_bins(indices, Dim::X, buffer2)} {}

  static Variable compute_on_buffer(const Variable &a, const Variable &b) {
    return compute_on_bin_buffer(a, b, 0, op);
  }

  Variable make_dense_bin_dims() {
    const auto bin_shape = indices.dims().shape();
    auto var = make_dense_variable<double>(
        Shape(std::vector(bin_shape.begin(), bin_shape.end())),
        std::get<bool>(GetParam()), 2.3, 4.02);
    var = var.rename_dims({{Dim::X, Dim{"i0"}}});
    if (var.dims().ndim() > 1)
      var = var.rename_dims({{Dim::Y, Dim{"i1"}}});
    if (var.dims().ndim() > 2)
      var = var.rename_dims({{Dim::Z, Dim{"i2"}}});
    return var;
  }
};

// Subset of tests involving broadcast, where the broadcast operand should not
// have variances.
class TransformBinaryIrregularBinsBroadcastTest
    : public TransformBinaryIrregularBinsTest {};

INSTANTIATE_TEST_SUITE_P(
    OneDIndices, TransformBinaryIrregularBinsTest,
    ::testing::Combine(::testing::ValuesIn(irregular_bin_indices_1d()),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    TwoDIndices, TransformBinaryIrregularBinsTest,
    ::testing::Combine(::testing::ValuesIn(irregular_bin_indices_2d()),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    OneDIndices, TransformBinaryIrregularBinsBroadcastTest,
    ::testing::Combine(::testing::ValuesIn(irregular_bin_indices_1d()),
                       ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(
    TwoDIndices, TransformBinaryIrregularBinsBroadcastTest,
    ::testing::Combine(::testing::ValuesIn(irregular_bin_indices_2d()),
                       ::testing::Values(false)));

TEST_P(TransformBinaryIrregularBinsTest, binned_with_binned) {
  const auto ab = transform<double>(binned1, binned2, op, name);
  EXPECT_EQ(ab, compute_on_buffer(binned1, binned2));

  transform_in_place<double>(binned1, binned2, op_in_place, name);
  EXPECT_EQ(binned1, ab);
}

TEST_P(TransformBinaryIrregularBinsBroadcastTest, binned_with_dense) {
  check_binned_with_dense(binned1, make_dense_bin_dims(), 0, op, op_in_place);
}

TEST_F(TransformBinaryTest, events_size_fail) {
  const auto indicesA = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{3, 4}});
  const auto indicesB = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 3}, std::pair{3, 3}});
  const auto table = makeVariable<double>(Dims{Dim::Event}, Shape{4});
  auto a = make_bins(indicesA, Dim::Event, table);
  auto b = make_bins(indicesB, Dim::Event, table);
  ASSERT_THROW_DISCARD(transform<pair_self_t<double>>(a, b, op, name),
                       except::BinnedDataError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, b, op_in_place, name),
               except::BinnedDataError);
}

TEST_F(TransformBinaryTest, inplace_nonbinned_lhs_binned_rhs) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 3}, std::pair{3, 3}});
  const auto table = makeVariable<double>(Dims{Dim::Event}, Shape{4});
  auto b = make_bins(indices, Dim::Event, table);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, b, op_in_place, name),
               except::BinnedDataError);
}

class TransformTest_events_binary_values_variances_size_fail
    : public ::testing::Test {
protected:
  Variable indicesA = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable indicesB = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 3}});
  Variable table =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{}, Variances{});
  Variable a = make_bins(indicesA, Dim::Event, table);
  Variable b = make_bins(indicesB, Dim::Event, table);
  Variable val_var = a;
  Variable val = make_bins(indicesA, Dim::Event, values(table));
  static constexpr auto op = [](const auto i, const auto j) { return i * j; };
  static constexpr auto op_in_place = [](auto &i, const auto j) { i *= j; };
};

TEST_F(TransformTest_events_binary_values_variances_size_fail, baseline) {
  ASSERT_NO_THROW_DISCARD(transform<pair_self_t<double>>(a, val_var, op, name));
  ASSERT_NO_THROW_DISCARD(transform<pair_self_t<double>>(a, val, op, name));
  ASSERT_NO_THROW(
      transform_in_place<pair_self_t<double>>(a, val_var, op_in_place, name));
  ASSERT_NO_THROW(
      transform_in_place<pair_self_t<double>>(a, val, op_in_place, name));
}

TEST_F(TransformTest_events_binary_values_variances_size_fail, a_size_bad) {
  a = b;
  ASSERT_THROW_DISCARD(transform<pair_self_t<double>>(a, val_var, op, name),
                       except::BinnedDataError);
  ASSERT_THROW_DISCARD(transform<pair_self_t<double>>(a, val, op, name),
                       except::BinnedDataError);
  ASSERT_THROW(
      transform_in_place<pair_self_t<double>>(a, val_var, op_in_place, name),
      except::BinnedDataError);
  ASSERT_THROW(
      transform_in_place<pair_self_t<double>>(a, val, op_in_place, name),
      except::BinnedDataError);
}
