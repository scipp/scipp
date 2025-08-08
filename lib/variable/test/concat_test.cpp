// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/variable/astype.h"
#include "scipp/variable/shape.h"

using namespace scipp;

class ConcatTest : public ::testing::Test {
protected:
  Variable base = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                       sc_units::m, Values{1, 2, 3, 4});
};

TEST_F(ConcatTest, unit_mismatch) {
  auto other = copy(base);
  other.setUnit(sc_units::s);
  EXPECT_THROW_DISCARD(concat(std::vector{base, other}, Dim::X),
                       except::UnitError);
}

TEST_F(ConcatTest, type_mismatch) {
  auto other = astype(base, dtype<int64_t>);
  EXPECT_THROW_DISCARD(concat(std::vector{base, other}, Dim::X),
                       except::TypeError);
}

TEST_F(ConcatTest, dimension_mismatch) {
  // Size mismatch
  EXPECT_THROW_DISCARD(
      concat(std::vector{base, base.slice({Dim::Y, 0, 1})}, Dim::X),
      except::DimensionError);
  // Label mismatch
  const auto xz = base.rename_dims({{Dim::Y, Dim::Z}});
  EXPECT_THROW_DISCARD(concat(std::vector{xz, base}, Dim::X),
                       except::DimensionError);
  // Missing label in first arg can (right now) not lead to broadcast
  EXPECT_THROW_DISCARD(
      concat(std::vector{base.slice({Dim::Y, 0}), base}, Dim::Z),
      except::DimensionError);
  // Missing label in second arg, but this broadcasts
  EXPECT_NO_THROW_DISCARD(
      concat(std::vector{base, base.slice({Dim::Y, 0})}, Dim::Z));
}

TEST_F(ConcatTest, new_dim) {
  EXPECT_EQ(
      concat(std::vector{base.slice({Dim::X, 0}), base.slice({Dim::X, 1})},
             Dim::X),
      base);
}

TEST_F(ConcatTest, new_dim_strided_inputs) {
  EXPECT_EQ(
      concat(std::vector{base.slice({Dim::Y, 0}), base.slice({Dim::Y, 1})},
             Dim::Y),
      transpose(base));
}

TEST_F(ConcatTest, existing_outer_dim) {
  auto expected =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{4, 2}, sc_units::m,
                           Values{1, 2, 3, 4, 2, 4, 6, 8});
  EXPECT_EQ(concat(std::vector{base, base + base}, Dim::X), expected);
}

TEST_F(ConcatTest, existing_inner_dim) {
  auto expected =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 4}, sc_units::m,
                           Values{1, 2, 2, 4, 3, 4, 6, 8});
  EXPECT_EQ(concat(std::vector{base, base + base}, Dim::Y), expected);
}

TEST_F(ConcatTest, existing_outer_transposed_other) {
  auto expected =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{4, 2}, sc_units::m,
                           Values{1, 2, 3, 4, 1, 2, 3, 4});
  EXPECT_EQ(concat(std::vector{base, copy(transpose(base))}, Dim::X), expected);
}

TEST_F(ConcatTest, existing_inner_transposed_other) {
  auto expected =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 4}, sc_units::m,
                           Values{1, 2, 1, 2, 3, 4, 3, 4});
  EXPECT_EQ(concat(std::vector{base, copy(transpose(base))}, Dim::Y), expected);
}

TEST_F(ConcatTest, existing_outer_dim_and_new_dim) {
  auto expected = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2},
                                       sc_units::m, Values{1, 2, 3, 4, 3, 4});
  EXPECT_EQ(concat(std::vector{base, base.slice({Dim::X, 1})}, Dim::X),
            expected);
}

TEST_F(ConcatTest, new_dim_and_existing_outer_dim) {
  auto expected = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2},
                                       sc_units::m, Values{3, 4, 1, 2, 3, 4});
  EXPECT_EQ(concat(std::vector{base.slice({Dim::X, 1}), base}, Dim::X),
            expected);
}

TEST_F(ConcatTest, empty) {
  EXPECT_THROW_DISCARD(concat(std::vector<Variable>{}, Dim::X),
                       std::invalid_argument);
}

TEST_F(ConcatTest, single_existing_dim) {
  auto out = concat(std::vector{base}, Dim::X);
  EXPECT_EQ(out, base);
  EXPECT_FALSE(out.is_same(base));
}

TEST_F(ConcatTest, single_new_dim) {
  auto out = concat(std::vector{base}, Dim::Z);
  EXPECT_EQ(out,
            broadcast(base, Dimensions({Dim::Z, Dim::X, Dim::Y}, {1, 2, 2})));
  EXPECT_FALSE(out.is_same(base));
}

TEST_F(ConcatTest, multiple) {
  EXPECT_EQ(concat(std::vector{base, base, base}, Dim::Z),
            broadcast(base, Dimensions({Dim::Z, Dim::X, Dim::Y}, {3, 2, 2})));
  auto a = base;
  auto b = base + base;
  auto c = base + base + base;
  for (const auto &dim : {Dim::X, Dim::Y, Dim::Z}) {
    auto abc = concat(std::vector{a, b, c}, dim);
    auto ab_c = concat(std::vector{concat(std::vector{a, b}, dim), c}, dim);
    auto a_bc = concat(std::vector{a, concat(std::vector{b, c}, dim)}, dim);
    EXPECT_EQ(abc, ab_c);
    EXPECT_EQ(abc, a_bc);
  }
}
