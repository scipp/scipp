// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/arg_list.h"

#include "scipp/variable/accumulate.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/variable.h"

#include "test_macros.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

namespace {
const char *name = "accumulate_test";
}

class AccumulateTest : public ::testing::Test {
protected:
  constexpr static auto op = [](auto &&a, auto &&b) { a += b; };
};

TEST_F(AccumulateTest, in_place) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                        Values{1.0, 2.0});
  const auto expected = makeVariable<double>(Values{3.0});
  // Note how accumulate is ignoring the unit.
  auto result = makeVariable<double>(Values{double{}});
  accumulate_in_place<pair_self_t<double>>(result, var, op, name);
  EXPECT_EQ(result, expected);
}

TEST_F(AccumulateTest, bad_dims) {
  const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                        sc_units::m, Values{1, 2, 3, 4, 5, 6});
  auto result = makeVariable<double>(Dims{Dim::X}, Shape{3});
  const auto orig = copy(result);
  EXPECT_THROW(accumulate_in_place<pair_self_t<double>>(result, var, op, name),
               except::DimensionError);
  EXPECT_EQ(result, orig);
}

TEST_F(AccumulateTest, broadcast) {
  const auto var = makeVariable<double>(Dims{Dim::Y}, Shape{3}, sc_units::m,
                                        Values{1, 2, 3});
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{6, 6});
  auto result = makeVariable<double>(Dims{Dim::X}, Shape{2});
  accumulate_in_place<pair_self_t<double>>(result, var, op, name);
  EXPECT_EQ(result, expected);
}

TEST_F(AccumulateTest, readonly) {
  const auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  scipp::index size = 10000; // exceed current lower multi-threading limit
  const auto readonly = broadcast(var, Dimensions({Dim::Y, Dim::X}, {size, 2}));
  auto result = makeVariable<double>(Dims{Dim::X}, Shape{2});
  accumulate_in_place<pair_self_t<double>>(result, readonly, op, name);
  EXPECT_EQ(result, var * makeVariable<double>(Values{size}));
}

auto make_variable(const Dimensions &dims) {
  const auto var = makeVariable<int64_t>(
      Dims{Dim("tmp")}, Shape{24},
      Values{1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
             13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24});
  return copy(fold(var, Dim("tmp"), dims));
}

TEST_F(AccumulateTest, 1d_to_scalar) {
  const auto var = make_variable({{Dim::X}, 24});
  const auto expected = makeVariable<int64_t>(Values{300});
  auto result = makeVariable<int64_t>(Values{0});
  accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
  EXPECT_EQ(result, expected);
}

TEST_F(AccumulateTest, 2d_to_scalar) {
  const auto var = make_variable({{Dim::X, Dim::Y}, {4, 6}});
  const auto expected = makeVariable<int64_t>(Values{300});
  auto result = makeVariable<int64_t>(Values{0});
  accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
  EXPECT_EQ(result, expected);
}

TEST_F(AccumulateTest, 2d_inner) {
  const auto var = make_variable({{Dim::X, Dim::Y}, {4, 6}});
  const auto expected =
      makeVariable<int64_t>(Dims{Dim::X}, Shape{4}, Values{21, 57, 93, 129});
  auto result = makeVariable<int64_t>(Dims{Dim::X}, Shape{4});
  accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
  EXPECT_EQ(result, expected);
}

TEST_F(AccumulateTest, 2d_outer) {
  const auto var = make_variable({{Dim::X, Dim::Y}, {4, 6}});
  const auto expected = makeVariable<int64_t>(Dims{Dim::Y}, Shape{6},
                                              Values{40, 44, 48, 52, 56, 60});
  auto result = makeVariable<int64_t>(Dims{Dim::Y}, Shape{6});
  accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
  EXPECT_EQ(result, expected);
}

//  1  2 |  7  8 | 13 14 | 19 20
//  3  4 |  9 10 | 15 16 | 21 22
//  5  6 | 11 12 | 17 18 | 23 24
TEST_F(AccumulateTest, 3d_inner) {
  const auto var = make_variable({{Dim::X, Dim::Y, Dim::Z}, {4, 3, 2}});
  auto result = makeVariable<int64_t>(Dims{Dim::X, Dim::Y}, Shape{4, 3});
  accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
  EXPECT_EQ(result, var.slice({Dim::Z, 0}) + var.slice({Dim::Z, 1}));
}

TEST_F(AccumulateTest, 3d_middle) {
  const auto var = make_variable({{Dim::X, Dim::Y, Dim::Z}, {4, 3, 2}});
  auto result = makeVariable<int64_t>(Dims{Dim::X, Dim::Z}, Shape{4, 2});
  accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
  EXPECT_EQ(result, var.slice({Dim::Y, 0}) + var.slice({Dim::Y, 1}) +
                        var.slice({Dim::Y, 2}));
}

TEST_F(AccumulateTest, 3d_outer) {
  const auto var = make_variable({{Dim::X, Dim::Y, Dim::Z}, {4, 3, 2}});
  const auto expected = makeVariable<int64_t>(Dims{Dim::Y, Dim::Z}, Shape{3, 2},
                                              Values{40, 44, 48, 52, 56, 60});
  auto result = makeVariable<int64_t>(Dims{Dim::Y, Dim::Z}, Shape{3, 2});
  accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
  EXPECT_EQ(result, expected);
}

TEST_F(AccumulateTest, 3d_middle_inner) {
  const auto var = make_variable({{Dim::X, Dim::Y, Dim::Z}, {4, 3, 2}});
  const auto expected =
      makeVariable<int64_t>(Dims{Dim::X}, Shape{4}, Values{21, 57, 93, 129});
  auto result = makeVariable<int64_t>(Dims{Dim::X}, Shape{4});
  accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
  EXPECT_EQ(result, expected);
}

TEST_F(AccumulateTest, 3d_outer_inner) {
  const auto var = make_variable({{Dim::X, Dim::Y, Dim::Z}, {4, 3, 2}});
  const auto expected =
      makeVariable<int64_t>(Dims{Dim::Y}, Shape{3}, Values{84, 100, 116});
  auto result = makeVariable<int64_t>(Dims{Dim::Y}, Shape{3});
  accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
  EXPECT_EQ(result, expected);
}

TEST_F(AccumulateTest, 3d_outer_middle) {
  const auto var = make_variable({{Dim::X, Dim::Y, Dim::Z}, {4, 3, 2}});
  const auto expected =
      makeVariable<int64_t>(Dims{Dim::Z}, Shape{2}, Values{144, 156});
  auto result = makeVariable<int64_t>(Dims{Dim::Z}, Shape{2});
  accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
  EXPECT_EQ(result, expected);
}

TEST_F(AccumulateTest, 1d_to_scalar_non_idempotent_init) {
  for (scipp::index i : {1, 7, 13, 31, 73, 99, 327, 1037, 7341, 8192, 45327}) {
    const auto var =
        broadcast(make_variable({{Dim::X}, 24}), {{Dim::X, Dim::Y}, {24, i}});
    const auto expected = makeVariable<int64_t>(Values{300 * i});
    auto result = makeVariable<int64_t>(Values{0});
    accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
    EXPECT_EQ(result, expected) << i;
    accumulate_in_place<pair_self_t<int64_t>>(result, var, op, name);
    EXPECT_EQ(result, 2 * sc_units::one * expected) << i;
  }
}
