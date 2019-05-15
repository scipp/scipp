// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "dimensions.h"
#include "except.h"
#include "variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(Variable, construct_default) {
  ASSERT_NO_THROW(Variable());
  Variable var;
  ASSERT_FALSE(var);
}

TEST(Variable, construct) {
  ASSERT_NO_THROW(makeVariable<double>(Dimensions(Dim::Tof, 2)));
  ASSERT_NO_THROW(makeVariable<double>(Dimensions(Dim::Tof, 2), 2));
  const auto a = makeVariable<double>(Dimensions(Dim::Tof, 2));
  const auto &data = a.values<double>();
  EXPECT_EQ(data.size(), 2);
}

TEST(Variable, construct_fail) {
  ASSERT_ANY_THROW(makeVariable<double>(Dimensions(), 2));
  ASSERT_ANY_THROW(makeVariable<double>(Dimensions(Dim::Tof, 1), 2));
  ASSERT_ANY_THROW(makeVariable<double>(Dimensions(Dim::Tof, 3), 2));
}

TEST(Variable, move) {
  auto var = makeVariable<double>({Dim::X, 2});
  Variable moved(std::move(var));
  // We need to define the behavior on move. Currently most methods will just
  // segfault, and we have no way of telling whether a Variable is in this
  // state.
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
  auto doubles = makeVariable<double>({Dim::X, 2}, {1, 2});
  auto ints = makeVariable<int32_t>({Dim::X, 2}, {1.1, 2.2});

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
  auto a = makeVariable<double>(Dimensions(Dim::Tof, 2));
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
  const auto base = makeVariable<double>({}, {1.1});
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>({}, {1.1}));
  expect_ne(base, makeVariable<double>({}, {1.2}));
}

TEST_F(Variable_comparison_operators, values_1d) {
  const auto base = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>({Dim::X, 2}, {1.1, 2.2}));
  expect_ne(base, makeVariable<double>({Dim::X, 2}, {1.1, 2.3}));
}

TEST_F(Variable_comparison_operators, values_2d) {
  const auto base =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 1}}, {1.1, 2.2});
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>({{Dim::X, 2}, {Dim::Y, 1}}, {1.1, 2.2}));
  expect_ne(base, makeVariable<double>({{Dim::X, 2}, {Dim::Y, 1}}, {1.1, 2.3}));
}

TEST_F(Variable_comparison_operators, variances_0d) {
  const auto base = makeVariable<double>({}, {1.1}, {0.1});
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>({}, {1.1}, {0.1}));
  expect_ne(base, makeVariable<double>({}, {1.1}));
  expect_ne(base, makeVariable<double>({}, {1.1}, {0.2}));
}

TEST_F(Variable_comparison_operators, variances_1d) {
  const auto base = makeVariable<double>({Dim::X, 2}, {1.1, 2.2}, {0.1, 0.2});
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>({Dim::X, 2}, {1.1, 2.2}, {0.1, 0.2}));
  expect_ne(base, makeVariable<double>({Dim::X, 2}, {1.1, 2.2}));
  expect_ne(base, makeVariable<double>({Dim::X, 2}, {1.1, 2.2}, {0.1, 0.3}));
}

TEST_F(Variable_comparison_operators, variances_2d) {
  const auto base =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 1}}, {1.1, 2.2}, {0.1, 0.2});
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>({{Dim::X, 2}, {Dim::Y, 1}}, {1.1, 2.2},
                                       {0.1, 0.2}));
  expect_ne(base, makeVariable<double>({{Dim::X, 2}, {Dim::Y, 1}}, {1.1, 2.2}));
  expect_ne(base, makeVariable<double>({{Dim::X, 2}, {Dim::Y, 1}}, {1.1, 2.2},
                                       {0.1, 0.3}));
}

TEST_F(Variable_comparison_operators, dimension_mismatch) {
  expect_ne(makeVariable<double>({}, {1.1}),
            makeVariable<double>({Dim::X, 1}, {1.1}));
  expect_ne(makeVariable<double>({Dim::X, 1}, {1.1}),
            makeVariable<double>({Dim::Y, 1}, {1.1}));
}

TEST_F(Variable_comparison_operators, dimension_transpose) {
  expect_ne(makeVariable<double>({{Dim::X, 1}, {Dim::Y, 1}}, {1.1}),
            makeVariable<double>({{Dim::Y, 1}, {Dim::X, 1}}, {1.1}));
}

TEST_F(Variable_comparison_operators, dimension_length) {
  expect_ne(makeVariable<double>({Dim::X, 1}),
            makeVariable<double>({Dim::X, 2}));
}

TEST_F(Variable_comparison_operators, unit) {
  const auto m = makeVariable<double>({Dim::X, 1}, units::m, {1.1});
  const auto s = makeVariable<double>({Dim::X, 1}, units::s, {1.1});
  expect_eq(m, m);
  expect_ne(m, s);
}

TEST_F(Variable_comparison_operators, dtype) {
  const auto base = makeVariable<double>({}, {1.0});
  expect_ne(base, makeVariable<float>({}, {1.0}));
}

TEST(VariableTest, copy_and_move) {
  const auto reference = makeVariable<double>({{Dim::X, 2}, {Dim::Y, 1}},
                                              units::m, {1.1, 2.2}, {0.1, 0.2});
  const auto var = makeVariable<double>({{Dim::X, 2}, {Dim::Y, 1}}, units::m,
                                        {1.1, 2.2}, {0.1, 0.2});

  const auto copy(var);
  EXPECT_EQ(copy, reference);

  const Variable copy_via_slice{VariableConstProxy(var)};
  EXPECT_EQ(copy_via_slice, reference);

  const auto moved(std::move(var));
  EXPECT_EQ(moved, reference);
}

TEST(Variable, setSlice) {
  Dimensions dims(Dim::Tof, 1);
  const auto parent = makeVariable<double>(
      Dimensions({{Dim::X, 4}, {Dim::Y, 2}, {Dim::Z, 3}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});
  const auto empty = makeVariable<double>(
      Dimensions({{Dim::X, 4}, {Dim::Y, 2}, {Dim::Z, 3}}), 24);

  auto d(empty);
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1, 2, 3})
    d(Dim::X, index).assign(parent(Dim::X, index));
  EXPECT_EQ(parent, d);

  d = empty;
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1})
    d(Dim::Y, index).assign(parent(Dim::Y, index));
  EXPECT_EQ(parent, d);

  d = empty;
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1, 2})
    d(Dim::Z, index).assign(parent(Dim::Z, index));
  EXPECT_EQ(parent, d);
}

TEST(Variable, slice) {
  Dimensions dims(Dim::Tof, 1);
  const auto parent = makeVariable<double>(
      Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 4}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});

  for (const scipp::index index : {0, 1, 2, 3}) {
    Variable sliceX = parent(Dim::X, index);
    ASSERT_EQ(sliceX.dims(), Dimensions({{Dim::Z, 3}, {Dim::Y, 2}}));
    auto base = static_cast<double>(index);
    EXPECT_EQ(sliceX.values<double>()[0], base + 1.0);
    EXPECT_EQ(sliceX.values<double>()[1], base + 5.0);
    EXPECT_EQ(sliceX.values<double>()[2], base + 9.0);
    EXPECT_EQ(sliceX.values<double>()[3], base + 13.0);
    EXPECT_EQ(sliceX.values<double>()[4], base + 17.0);
    EXPECT_EQ(sliceX.values<double>()[5], base + 21.0);
  }

  for (const scipp::index index : {0, 1}) {
    Variable sliceY = parent(Dim::Y, index);
    ASSERT_EQ(sliceY.dims(), Dimensions({{Dim::Z, 3}, {Dim::X, 4}}));
    const auto &data = sliceY.values<double>();
    auto base = static_cast<double>(index);
    for (const scipp::index z : {0, 1, 2}) {
      EXPECT_EQ(data[4 * z + 0], 4 * base + 8 * static_cast<double>(z) + 1.0);
      EXPECT_EQ(data[4 * z + 1], 4 * base + 8 * static_cast<double>(z) + 2.0);
      EXPECT_EQ(data[4 * z + 2], 4 * base + 8 * static_cast<double>(z) + 3.0);
      EXPECT_EQ(data[4 * z + 3], 4 * base + 8 * static_cast<double>(z) + 4.0);
    }
  }

  for (const scipp::index index : {0, 1, 2}) {
    Variable sliceZ = parent(Dim::Z, index);
    ASSERT_EQ(sliceZ.dims(), Dimensions({{Dim::Y, 2}, {Dim::X, 4}}));
    const auto &data = sliceZ.values<double>();
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
  }
}

TEST(Variable, slice_range) {
  Dimensions dims(Dim::Tof, 1);
  const auto parent = makeVariable<double>(
      Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 4}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});

  for (const scipp::index index : {0, 1, 2, 3}) {
    Variable sliceX = parent(Dim::X, index, index + 1);
    ASSERT_EQ(sliceX.dims(),
              Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 1}}));
    EXPECT_EQ(sliceX.values<double>()[0], index + 1.0);
    EXPECT_EQ(sliceX.values<double>()[1], index + 5.0);
    EXPECT_EQ(sliceX.values<double>()[2], index + 9.0);
    EXPECT_EQ(sliceX.values<double>()[3], index + 13.0);
    EXPECT_EQ(sliceX.values<double>()[4], index + 17.0);
    EXPECT_EQ(sliceX.values<double>()[5], index + 21.0);
  }

  for (const scipp::index index : {0, 1, 2}) {
    Variable sliceX = parent(Dim::X, index, index + 2);
    ASSERT_EQ(sliceX.dims(),
              Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 2}}));
    EXPECT_EQ(sliceX.values<double>()[0], index + 1.0);
    EXPECT_EQ(sliceX.values<double>()[1], index + 2.0);
    EXPECT_EQ(sliceX.values<double>()[2], index + 5.0);
    EXPECT_EQ(sliceX.values<double>()[3], index + 6.0);
    EXPECT_EQ(sliceX.values<double>()[4], index + 9.0);
    EXPECT_EQ(sliceX.values<double>()[5], index + 10.0);
    EXPECT_EQ(sliceX.values<double>()[6], index + 13.0);
    EXPECT_EQ(sliceX.values<double>()[7], index + 14.0);
    EXPECT_EQ(sliceX.values<double>()[8], index + 17.0);
    EXPECT_EQ(sliceX.values<double>()[9], index + 18.0);
    EXPECT_EQ(sliceX.values<double>()[10], index + 21.0);
    EXPECT_EQ(sliceX.values<double>()[11], index + 22.0);
  }

  for (const scipp::index index : {0, 1}) {
    Variable sliceY = parent(Dim::Y, index, index + 1);
    ASSERT_EQ(sliceY.dims(),
              Dimensions({{Dim::Z, 3}, {Dim::Y, 1}, {Dim::X, 4}}));
    const auto &data = sliceY.values<double>();
    for (const scipp::index z : {0, 1, 2}) {
      EXPECT_EQ(data[4 * z + 0], 4 * index + 8 * z + 1.0);
      EXPECT_EQ(data[4 * z + 1], 4 * index + 8 * z + 2.0);
      EXPECT_EQ(data[4 * z + 2], 4 * index + 8 * z + 3.0);
      EXPECT_EQ(data[4 * z + 3], 4 * index + 8 * z + 4.0);
    }
  }

  for (const scipp::index index : {0}) {
    Variable sliceY = parent(Dim::Y, index, index + 2);
    EXPECT_EQ(sliceY, parent);
  }

  for (const scipp::index index : {0, 1, 2}) {
    Variable sliceZ = parent(Dim::Z, index, index + 1);
    ASSERT_EQ(sliceZ.dims(),
              Dimensions({{Dim::Z, 1}, {Dim::Y, 2}, {Dim::X, 4}}));
    const auto &data = sliceZ.values<double>();
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
  }

  for (const scipp::index index : {0, 1}) {
    Variable sliceZ = parent(Dim::Z, index, index + 2);
    ASSERT_EQ(sliceZ.dims(),
              Dimensions({{Dim::Z, 2}, {Dim::Y, 2}, {Dim::X, 4}}));
    const auto &data = sliceZ.values<double>();
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[8 + xy], 1.0 + 8 + xy + 8 * index);
  }
}

TEST(Variable, broadcast) {
  auto reference = makeVariable<double>({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 2}},
                                        {1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4});
  auto var = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});

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
  auto var = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  EXPECT_THROW_MSG(broadcast(var, {Dim::X, 3}), except::DimensionLengthError,
                   "Expected dimension to be in {{Dim::Y, 2}, {Dim::X, 2}}, "
                   "got Dim::X with mismatching length 3.");
}

TEST(VariableProxy, full_const_view) {
  const auto var = makeVariable<double>({{Dim::X, 3}});
  VariableConstProxy view(var);
  EXPECT_EQ(var.values<double>().data(), view.values<double>().data());
}

TEST(VariableProxy, full_mutable_view) {
  auto var = makeVariable<double>({{Dim::X, 3}});
  VariableProxy view(var);
  EXPECT_EQ(var.values<double>().data(), view.values<double>().data());
  EXPECT_EQ(var.values<double>().data(), view.values<double>().data());
}

TEST(VariableProxy, strides) {
  auto var = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
  EXPECT_EQ(var(Dim::X, 0).strides(), (std::vector<scipp::index>{3}));
  EXPECT_EQ(var(Dim::X, 1).strides(), (std::vector<scipp::index>{3}));
  EXPECT_EQ(var(Dim::Y, 0).strides(), (std::vector<scipp::index>{1}));
  EXPECT_EQ(var(Dim::Y, 1).strides(), (std::vector<scipp::index>{1}));
  EXPECT_EQ(var(Dim::X, 0, 1).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::X, 1, 2).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 0, 1).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 1, 2).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::X, 0, 2).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::X, 1, 3).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 0, 2).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 1, 3).strides(), (std::vector<scipp::index>{3, 1}));

  EXPECT_EQ(var(Dim::X, 0, 1)(Dim::Y, 0, 1).strides(),
            (std::vector<scipp::index>{3, 1}));

  auto var3D = makeVariable<double>({{Dim::Z, 4}, {Dim::Y, 3}, {Dim::X, 2}});
  EXPECT_EQ(var3D(Dim::X, 0, 1)(Dim::Z, 0, 1).strides(),
            (std::vector<scipp::index>{6, 2, 1}));
}

TEST(VariableProxy, get) {
  const auto var = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  EXPECT_EQ(var(Dim::X, 1, 2).values<double>()[0], 2.0);
}

TEST(VariableProxy, slicing_does_not_transpose) {
  auto var = makeVariable<double>({{Dim::X, 3}, {Dim::Y, 3}});
  Dimensions expected{{Dim::X, 1}, {Dim::Y, 1}};
  EXPECT_EQ(var(Dim::X, 1, 2)(Dim::Y, 1, 2).dims(), expected);
  EXPECT_EQ(var(Dim::Y, 1, 2)(Dim::X, 1, 2).dims(), expected);
}

TEST(VariableProxy, variable_copy_from_slice) {
  const auto source = makeVariable<double>(
      {{Dim::Y, 3}, {Dim::X, 3}}, {11, 12, 13, 21, 22, 23, 31, 32, 33});

  Variable target1(source(Dim::X, 0, 2)(Dim::Y, 0, 2));
  EXPECT_EQ(target1.dims(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target1.values<double>(), {11, 12, 21, 22}));

  Variable target2(source(Dim::X, 1, 3)(Dim::Y, 0, 2));
  EXPECT_EQ(target2.dims(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target2.values<double>(), {12, 13, 22, 23}));

  Variable target3(source(Dim::X, 0, 2)(Dim::Y, 1, 3));
  EXPECT_EQ(target3.dims(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target3.values<double>(), {21, 22, 31, 32}));

  Variable target4(source(Dim::X, 1, 3)(Dim::Y, 1, 3));
  EXPECT_EQ(target4.dims(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target4.values<double>(), {22, 23, 32, 33}));
}

TEST(VariableProxy, variable_assign_from_slice) {
  auto target = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  const auto source = makeVariable<double>(
      {{Dim::Y, 3}, {Dim::X, 3}}, {11, 12, 13, 21, 22, 23, 31, 32, 33});

  target = source(Dim::X, 0, 2)(Dim::Y, 0, 2);
  EXPECT_EQ(target.dims(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.values<double>(), {11, 12, 21, 22}));

  target = source(Dim::X, 1, 3)(Dim::Y, 0, 2);
  EXPECT_EQ(target.dims(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.values<double>(), {12, 13, 22, 23}));

  target = source(Dim::X, 0, 2)(Dim::Y, 1, 3);
  EXPECT_EQ(target.dims(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.values<double>(), {21, 22, 31, 32}));

  target = source(Dim::X, 1, 3)(Dim::Y, 1, 3);
  EXPECT_EQ(target.dims(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.values<double>(), {22, 23, 32, 33}));
}

TEST(VariableProxy, variable_self_assign_via_slice) {
  auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}},
                                     {11, 12, 13, 21, 22, 23, 31, 32, 33});

  target = target(Dim::X, 1, 3)(Dim::Y, 1, 3);
  // Note: This test does not actually fail if self-assignment is broken. Had to
  // run address sanitizer to see that it is reading from free'ed memory.
  EXPECT_EQ(target.dims(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.values<double>(), {22, 23, 32, 33}));
}

TEST(VariableProxy, slice_assign_from_variable) {
  const auto source =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {11, 12, 21, 22});

  // We might want to mimick Python's __setitem__, but operator= would (and
  // should!?) assign the view contents, not the data.
  {
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 0, 2)(Dim::Y, 0, 2).assign(source);
    EXPECT_EQ(target.dims(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(
        equals(target.values<double>(), {11, 12, 0, 21, 22, 0, 0, 0, 0}));
  }
  {
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 1, 3)(Dim::Y, 0, 2).assign(source);
    EXPECT_EQ(target.dims(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(
        equals(target.values<double>(), {0, 11, 12, 0, 21, 22, 0, 0, 0}));
  }
  {
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 0, 2)(Dim::Y, 1, 3).assign(source);
    EXPECT_EQ(target.dims(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(
        equals(target.values<double>(), {0, 0, 0, 11, 12, 0, 21, 22, 0}));
  }
  {
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 1, 3)(Dim::Y, 1, 3).assign(source);
    EXPECT_EQ(target.dims(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(
        equals(target.values<double>(), {0, 0, 0, 0, 11, 12, 0, 21, 22}));
  }
}

TEST(VariableTest, reshape) {
  const auto var =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6});

  ASSERT_EQ(var.reshape({Dim::Row, 6}),
            makeVariable<double>({Dim::Row, 6}, {1, 2, 3, 4, 5, 6}));
  ASSERT_EQ(
      var.reshape({{Dim::Row, 3}, {Dim::Z, 2}}),
      makeVariable<double>({{Dim::Row, 3}, {Dim::Z, 2}}, {1, 2, 3, 4, 5, 6}));
}

TEST(VariableTest, reshape_with_variance) {
  const auto var = makeVariable<double>(
      {{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6}, {7, 8, 9, 10, 11, 12});

  ASSERT_EQ(var.reshape({Dim::Row, 6}),
            makeVariable<double>({Dim::Row, 6}, {1, 2, 3, 4, 5, 6},
                                 {7, 8, 9, 10, 11, 12}));
  ASSERT_EQ(var.reshape({{Dim::Row, 3}, {Dim::Z, 2}}),
            makeVariable<double>({{Dim::Row, 3}, {Dim::Z, 2}},
                                 {1, 2, 3, 4, 5, 6}, {7, 8, 9, 10, 11, 12}));
}

TEST(VariableTest, reshape_temporary) {
  const auto var = makeVariable<double>({{Dim::X, 2}, {Dim::Row, 4}},
                                        {1, 2, 3, 4, 5, 6, 7, 8});
  auto reshaped = sum(var, Dim::X).reshape({{Dim::Y, 2}, {Dim::Z, 2}});
  ASSERT_EQ(reshaped,
            makeVariable<double>({{Dim::Y, 2}, {Dim::Z, 2}}, {6, 8, 10, 12}));

  // This is not a temporary, we get a view into `var`.
  EXPECT_EQ(typeid(decltype(std::move(var).reshape({}))),
            typeid(VariableConstProxy));
}

TEST(VariableTest, reshape_fail) {
  auto var =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6});
  EXPECT_THROW_MSG(var.reshape({Dim::Row, 5}), std::runtime_error,
                   "Cannot reshape to dimensions with different volume");
}

TEST(VariableTest, reshape_and_slice) {
  auto var =
      makeVariable<double>({Dim::Spectrum, 16}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                                 11, 12, 13, 14, 15, 16});

  auto slice =
      var.reshape({{Dim::X, 4}, {Dim::Y, 4}})(Dim::X, 1, 3)(Dim::Y, 1, 3);
  ASSERT_EQ(slice,
            makeVariable<double>({{Dim::X, 2}, {Dim::Y, 2}}, {6, 7, 10, 11}));

  Variable center =
      var.reshape({{Dim::X, 4}, {Dim::Y, 4}})(Dim::X, 1, 3)(Dim::Y, 1, 3)
          .reshape({Dim::Spectrum, 4});

  ASSERT_EQ(center, makeVariable<double>({Dim::Spectrum, 4}, {6, 7, 10, 11}));
}

TEST(VariableTest, reshape_mutable) {
  auto modified_original =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 0, 5, 6});
  auto reference = makeVariable<double>({Dim::Row, 6}, {1, 2, 3, 0, 5, 6});

  auto var =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6});

  auto view = var.reshape({Dim::Row, 6});
  view.values<double>()[3] = 0;

  ASSERT_EQ(view, reference);
  ASSERT_EQ(var, modified_original);
}

TEST(Variable, access_typed_view) {
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, {1, 2, 3, 4, 5, 6});
  const auto values =
      getView<double>(var, {{Dim::Y, 2}, {Dim::Z, 4}, {Dim::X, 3}});
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
  auto var =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6});
  const auto values =
      getView<double>(var, {{Dim::Y, 2}, {Dim::Z, 4}, {Dim::X, 2}});
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
  const auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  EXPECT_TRUE(var.isSparse());
  EXPECT_EQ(var.sparseDim(), Dim::X);
  // Should we return the full volume here, i.e., accumulate the extents of all
  // the sparse subdata?
  EXPECT_EQ(var.dims().volume(), 2);
}

TEST(SparseVariable, dtype) {
  const auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  // It is not clear that this is the best way of handling things.
  // Variable::dtype() makes sense like this, but it is not so clear for
  // VariableConcept::dtype().
  EXPECT_EQ(var.dtype(), dtype<double>);
  EXPECT_NE(var.data().dtype(), dtype<double>);
}

TEST(SparseVariable, non_sparse_access_fail) {
  const auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  ASSERT_THROW(var.values<double>(), except::TypeError);
  ASSERT_THROW(var.values<double>(), except::TypeError);
}

TEST(SparseVariable, DISABLED_low_level_access) {
  const auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  // Need to decide whether we allow this direct access or not.
  ASSERT_THROW((var.values<sparse_container<double>>()), except::TypeError);
}

TEST(SparseVariable, access) {
  const auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  ASSERT_NO_THROW(var.sparseSpan<double>());
  auto data = var.sparseSpan<double>();
  ASSERT_EQ(data.size(), 2);
  EXPECT_TRUE(data[0].empty());
  EXPECT_TRUE(data[1].empty());
}

TEST(SparseVariable, resize_sparse) {
  auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto data = var.sparseSpan<double>();
  data[1] = {1, 2, 3};
}

TEST(SparseVariable, comparison) {
  auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};
  auto b = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto b_ = b.sparseSpan<double>();
  b_[0] = {1, 2, 3};
  b_[1] = {1, 2};
  auto c = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto c_ = c.sparseSpan<double>();
  c_[0] = {1, 3};
  c_[1] = {};

  EXPECT_EQ(a, a);
  EXPECT_EQ(a, b);
  EXPECT_EQ(b, a);

  EXPECT_NE(a, c);
  EXPECT_NE(c, a);
}

TEST(SparseVariable, copy) {
  auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};

  Variable copy(a);
  EXPECT_EQ(a, copy);
}

TEST(SparseVariable, move) {
  auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};

  Variable copy(a);
  Variable moved(std::move(copy));
  EXPECT_EQ(a, moved);
}

TEST(SparseVariable, concatenate) {
  const auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  const auto b = makeSparseVariable<double>({Dim::Y, 3}, Dim::X);
  auto var = concatenate(a, b, Dim::Y);
  EXPECT_TRUE(var.isSparse());
  EXPECT_EQ(var.sparseDim(), Dim::X);
  EXPECT_EQ(var.dims().volume(), 5);
}

TEST(SparseVariable, concatenate_along_sparse_dimension) {
  auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};
  auto b = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto b_ = b.sparseSpan<double>();
  b_[0] = {1, 3};
  b_[1] = {};

  auto var = concatenate(a, b, Dim::X);
  EXPECT_TRUE(var.isSparse());
  EXPECT_EQ(var.sparseDim(), Dim::X);
  EXPECT_EQ(var.dims().volume(), 2);
  auto data = var.sparseSpan<double>();
  EXPECT_TRUE(equals(data[0], {1, 2, 3, 1, 3}));
  EXPECT_TRUE(equals(data[1], {1, 2}));
}

TEST(SparseVariable, slice) {
  auto var = makeSparseVariable<double>({Dim::Y, 4}, Dim::X);
  auto data = var.sparseSpan<double>();
  data[0] = {1, 2, 3};
  data[1] = {1, 2};
  data[2] = {1};
  data[3] = {};
  auto slice = var(Dim::Y, 1, 3);
  EXPECT_TRUE(slice.isSparse());
  EXPECT_EQ(slice.sparseDim(), Dim::X);
  EXPECT_EQ(slice.dims().volume(), 2);
  auto slice_data = slice.sparseSpan<double>();
  EXPECT_TRUE(equals(slice_data[0], {1, 2}));
  EXPECT_TRUE(equals(slice_data[1], {1}));
}

TEST(SparseVariable, operator_plus) {
  auto sparse = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto sparse_ = sparse.sparseSpan<double>();
  sparse_[0] = {1, 2, 3};
  sparse_[1] = {4};
  auto dense = makeVariable<double>({Dim::Y, 2}, {1.5, 0.5});

  sparse += dense;

  EXPECT_TRUE(equals(sparse_[0], {2.5, 3.5, 4.5}));
  EXPECT_TRUE(equals(sparse_[1], {4.5}));
}

TEST(VariableTest, create_with_variance) {
  ASSERT_NO_THROW(makeVariable<double>({}, {1.0}, {0.1}));
  ASSERT_NO_THROW(makeVariable<double>({}, units::m, {1.0}, {0.1}));
}

TEST(VariableTest, hasVariances) {
  ASSERT_FALSE(makeVariable<double>({}).hasVariances());
  ASSERT_FALSE(makeVariable<double>({}, {1.0}).hasVariances());
  ASSERT_TRUE(makeVariable<double>({}, {1.0}, {0.1}).hasVariances());
  ASSERT_TRUE(makeVariable<double>({}, units::m, {1.0}, {0.1}).hasVariances());
}

TEST(VariableTest, values_variances) {
  const auto var = makeVariable<double>({}, {1.0}, {0.1});
  ASSERT_NO_THROW(var.values<double>());
  ASSERT_NO_THROW(var.variances<double>());
  ASSERT_TRUE(equals(var.values<double>(), {1.0}));
  ASSERT_TRUE(equals(var.variances<double>(), {0.1}));
}

TEST(VariableProxyTest, create_with_variance) {
  const auto var = makeVariable<double>({Dim::X, 2}, {1.0, 2.0}, {0.1, 0.2});
  ASSERT_NO_THROW(var.slice({Dim::X, 1, 2}));
  const auto slice = var.slice({Dim::X, 1, 2});
  ASSERT_TRUE(slice.hasVariances());
  ASSERT_EQ(slice.variances<double>().size(), 1);
  ASSERT_EQ(slice.variances<double>()[0], 0.2);
  const auto reference = makeVariable<double>({Dim::X, 1}, {2.0}, {0.2});
  ASSERT_EQ(slice, reference);
}
