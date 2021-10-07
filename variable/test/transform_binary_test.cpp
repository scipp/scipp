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
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

#include "test_variables.h"

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

class DenseTransformBinaryTest
    : public TransformBinaryTest,
      public ::testing::WithParamInterface<std::tuple<Shape, bool>> {
protected:
  const Variable input1;
  const Variable input2;

  DenseTransformBinaryTest()
      : input1{make_input_variable(0, 1)}, input2{make_input_variable(10, 2)} {}

  static Variable make_input_variable(const double offset, const double scale) {
    const auto &[shape, variances] = GetParam();
    return make_dense_variable<double>(shape, variances, offset, scale);
  }

  void check_transform_combinations(const Variable &a, const Variable &b) {
    const auto b_for_manual = b.dims() == a.dims() ? b : b.broadcast(a.dims());

    const auto ab = transform<pair_self_t<double>>(a, b, op, name);
    EXPECT_TRUE(equals(ab.values<double>(),
                       op_manual_values<double, double>(a, b_for_manual)));
    if (std::get<bool>(GetParam())) {
      EXPECT_TRUE(equals(ab.variances<double>(),
                         op_manual_variances<double, double>(a, b_for_manual)));
    }

    const auto ba = transform<pair_self_t<double>>(b, a, op, name);
    EXPECT_TRUE(equals(ba.values<double>(),
                       op_manual_values<double, double>(b_for_manual, a)));
    if (std::get<bool>(GetParam())) {
      EXPECT_TRUE(equals(ba.variances<double>(),
                         op_manual_variances<double, double>(b_for_manual, a)));
    }

    auto result_in_place = copy(a);
    transform_in_place<pair_self_t<double>>(result_in_place, b, op_in_place,
                                            name);
    EXPECT_EQ(result_in_place, ab);
  }
};

INSTANTIATE_TEST_SUITE_P(Array, DenseTransformBinaryTest,
                         ::testing::Combine(::testing::ValuesIn(shapes),
                                            ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(Scalar, DenseTransformBinaryTest,
                         ::testing::Combine(::testing::Values(Shape{0}),
                                            ::testing::Bool()));

TEST_P(DenseTransformBinaryTest, matching_shapes) {
  check_transform_combinations(input1, input2);
}

TEST_P(DenseTransformBinaryTest, scalar_and_array) {
  const auto s = std::get<bool>(GetParam())
                     ? makeVariable<double>(Values{2.1}, Variances{1.3})
                     : makeVariable<double>(Values{2.1});
  check_transform_combinations(input1, s);
}

TEST_P(DenseTransformBinaryTest, slice_and_slice) {
  for (const Slice &slice : make_slices(input1.dims().shape())) {
    const auto a = input1.slice(slice);
    const auto b = input2.slice(slice);
    check_transform_combinations(a, b);
    // Make one input a full view of its data.
    check_transform_combinations(copy(a), b);
  }
}

TEST_P(DenseTransformBinaryTest, slice_and_full) {
  for (const Slice &slice : make_slices(input1.dims().shape())) {
    const auto a = input1.slice(slice);
    auto b = copy(a);
    std::generate(b.values<double>().begin(), b.values<double>().end(),
                  [x = 0.0, size = b.dims().volume()]() mutable {
                    return x += static_cast<double>(size) / 4.0;
                  });

    check_transform_combinations(a, b);
  }
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

TEST_F(TransformBinaryTest, transpose) {
  auto a =
      makeVariable<int>(Dims{Dim::X, Dim::Y}, Shape{2, 2}, Values{1, 2, 3, 4});
  const auto b = transpose(
      makeVariable<int>(Dims{Dim::X, Dim::Y}, Shape{2, 2}, Values{5, 6, 7, 8}),
      {Dim::Y, Dim::X});

  const auto ab = transform<int>(a, b, op, name);
  transform_in_place<int>(a, b, op_in_place, name);

  EXPECT_TRUE(equals(a.values<int>(), {5, 12, 21, 32}));
  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinaryTest, transposed_layout) {
  auto a =
      makeVariable<int>(Dims{Dim::X, Dim::Y}, Shape{2, 2}, Values{1, 2, 3, 4});
  const auto b =
      makeVariable<int>(Dims{Dim::Y, Dim::X}, Shape{2, 2}, Values{5, 7, 6, 8});

  const auto ab = transform<int>(a, b, op, name);
  transform_in_place<int>(a, b, op_in_place, name);

  EXPECT_TRUE(equals(a.values<int>(), {5, 12, 21, 32}));
  EXPECT_EQ(ab, a);
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
  Variable slice_copy(a.slice({Dim::X, 1}));
  auto reference = a * slice_copy;
  // With self-overlap the implementation needs to make a copy of the rhs. This
  // is a regression test: An initial implementation was unintentionally
  // dropping the variances when making that copy.
  transform_in_place<pair_self_t<double>>(a, a.slice({Dim::X, 1}), op_in_place,
                                          name);
  ASSERT_EQ(a, reference);
}

TEST_F(TransformBinaryTest, view_with_var) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  const auto b = makeVariable<double>(Values{3.3});

  transform_in_place<pair_self_t<double>>(a.slice({Dim::X, 1}), b, op_in_place,
                                          name);

  EXPECT_TRUE(equals(a.values<double>(), {1.1, 2.2 * 3.3}));
}

TEST_F(TransformBinaryTest, view_with_view) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  const auto b = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{0.1, 3.3});

  transform_in_place<pair_self_t<double>>(
      a.slice({Dim::X, 1}), b.slice({Dim::Y, 1}), op_in_place, name);

  EXPECT_TRUE(equals(a.values<double>(), {1.1, 2.2 * 3.3}));
}

TEST_F(TransformBinaryTest, dense_events) {
  const auto indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 3}, std::pair{3, 4}});
  const auto table =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4});
  auto events = make_bins(indices, Dim::Event, copy(table));
  auto dense = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.5, 0.5});

  const auto ab = transform<pair_self_t<double>>(events, dense, op, name);
  const auto ba = transform<pair_self_t<double>>(dense, events, op, name);
  transform_in_place<pair_self_t<double>>(events, dense, op_in_place, name);

  const auto expected = transform<double>(events, dense, op, name);
  EXPECT_EQ(events.values<bucket<Variable>>()[0],
            transform<pair_self_t<double>>(dense.slice({Dim::X, 0}),
                                           table.slice({Dim::Event, 0, 3}), op,
                                           name));
  EXPECT_EQ(events.values<bucket<Variable>>()[1],
            transform<pair_self_t<double>>(dense.slice({Dim::X, 1}),
                                           table.slice({Dim::Event, 3, 4}), op,
                                           name));
  EXPECT_EQ(ab, events);
  EXPECT_EQ(ba, events);
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

TEST_F(TransformBinaryTest, in_place_unit_change) {
  const auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{1.0, 2.0});
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{2},
                           units::Unit(units::m * units::m), Values{1.0, 4.0});
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
                overloaded{
                    [](const units::Unit &a, const units::Unit &) { return a; },
                    [](const auto x, const auto y) { return !x || y; }},
                name),
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, true}));

  transform_in_place<bool>(
      var, overloaded{[](units::Unit &) {}, [](auto &x) { x = !x; }}, name);
  EXPECT_EQ(var,
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true}));

  transform_in_place<pair_self_t<bool>>(
      var, var,
      overloaded{[](units::Unit &, const units::Unit &) {},
                 [](auto &x, const auto &y) { x = !x || y; }},
      name);
  EXPECT_EQ(var,
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, true}));
}

class TransformBinsBinaryTest : public TransformBinaryTest {
protected:
  Variable indicesY = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::Y}, Shape{2}, Values{std::pair{0, 3}, std::pair{3, 4}});
  Variable tableA =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, units::m,
                           Values{1, 2, 3, 4}, Variances{5, 6, 7, 8});
  Variable tableB = makeVariable<double>(Dims{Dim::Event}, Shape{4},
                                         Values{0.1, 0.2, 0.3, 0.4},
                                         Variances{0.5, 0.6, 0.7, 0.8});
  Variable a = make_bins(indicesY, Dim::Event, copy(tableA));
  Variable b = make_bins(indicesY, Dim::Event, copy(tableB));
};

TEST_F(TransformBinsBinaryTest, events_val_var_with_events_val_var) {
  const auto ab = transform<pair_self_t<double>>(a, b, op, name);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place, name);
  // We rely on correctness of *dense* operations (Variable multiplication is
  // also built on transform).
  EXPECT_EQ(a, make_bins(indicesY, Dim::Event, tableA * tableB));
  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinsBinaryTest, events_val_var_with_events_val) {
  const auto table = values(tableB);
  b = make_bins(indicesY, Dim::Event, table);
  const auto ab = transform<pair_self_t<double>>(a, b, op, name);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place, name);
  EXPECT_EQ(a, make_bins(indicesY, Dim::Event, tableA * table));
  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinsBinaryTest, events_val_var_with_val_var) {
  b = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1.5, 1.6},
                           Variances{1.7, 1.8});

  const auto ab = transform<pair_self_t<double>>(a, b, op, name);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place, name);

  tableA.slice({Dim::Event, 0, 3}) *= b.slice({Dim::Y, 0});
  tableA.slice({Dim::Event, 3, 4}) *= b.slice({Dim::Y, 1});
  EXPECT_EQ(a, make_bins(indicesY, Dim::Event, tableA));
  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinsBinaryTest, events_val_var_with_val) {
  b = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1.5, 1.6});

  const auto ab = transform<pair_self_t<double>>(a, b, op, name);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place, name);

  tableA.slice({Dim::Event, 0, 3}) *= b.slice({Dim::Y, 0});
  tableA.slice({Dim::Event, 3, 4}) *= b.slice({Dim::Y, 1});
  EXPECT_EQ(a, make_bins(indicesY, Dim::Event, tableA));
  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinsBinaryTest, broadcast_events_val_var_with_val) {
  b = makeVariable<float>(Dims{Dim::Z}, Shape{2}, Values{1.5, 1.6});

  const auto ab =
      transform<pair_custom_t<std::tuple<double, float>>>(a, b, op, name);

  EXPECT_EQ(ab.slice({Dim::Z, 0}),
            make_bins(indicesY, Dim::Event, tableA * b.slice({Dim::Z, 0})));
  EXPECT_EQ(ab.slice({Dim::Z, 1}),
            make_bins(indicesY, Dim::Event, tableA * b.slice({Dim::Z, 1})));
  EXPECT_EQ(ab.dims(), Dimensions({Dim::Y, Dim::Z}, {2, 2}));
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
