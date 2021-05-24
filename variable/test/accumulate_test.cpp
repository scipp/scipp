// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/arg_list.h"

#include "scipp/variable/accumulate.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/variable.h"

#include "test_macros.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

TEST(AccumulateTest, in_place) {
  const auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{1.0, 2.0});
  const auto expected = makeVariable<double>(Values{3.0});
  auto op_ = [](auto &&a, auto &&b) { a += b; };
  // Note how accumulate is ignoring the unit.
  auto result = makeVariable<double>(Values{double{}});
  accumulate_in_place<pair_self_t<double>>(result, var, op_);
  EXPECT_EQ(result, expected);
}

TEST(AccumulateTest, bad_dims) {
  const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                        units::m, Values{1, 2, 3, 4, 5, 6});
  auto op_ = [](auto &&a, auto &&b) { a += b; };
  auto result = makeVariable<double>(Dims{Dim::X}, Shape{3});
  const auto orig = copy(result);
  EXPECT_THROW(accumulate_in_place<pair_self_t<double>>(result, var, op_),
               except::DimensionError);
  EXPECT_EQ(result, orig);
}

TEST(AccumulateTest, broadcast) {
  const auto var =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, units::m, Values{1, 2, 3});
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{6, 6});
  auto op_ = [](auto &&a, auto &&b) { a += b; };
  auto result = makeVariable<double>(Dims{Dim::X}, Shape{2});
  accumulate_in_place<pair_self_t<double>>(result, var, op_);
  EXPECT_EQ(result, expected);
}
