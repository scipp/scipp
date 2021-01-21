// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/reduction.h"
#include "scipp/variable/string.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

class SumTest : public ::testing::Test {
protected:
  Variable var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                      units::m, Values{1.0, 2.0, 3.0, 4.0});
  Variable var_bool =
      makeVariable<bool>(Dims{Dim::Y, Dim::X}, Shape{2, 2}, units::m,
                         Values{true, false, true, true});
};

TEST_F(SumTest, sum) {
  const auto expectedX =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{3.0, 7.0});
  const auto expectedY =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{4.0, 6.0});
  EXPECT_EQ(sum(var, Dim::X), expectedX);
}

TEST_F(SumTest, sum_with_empty_dim) {
  const auto empty_slice = var.slice({Dim::X, 0, 0});
  EXPECT_EQ(
      sum(empty_slice, Dim::X),
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{0, 0}));
  EXPECT_EQ(sum(empty_slice, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{0}, units::m, Values{}));
}

TEST_F(SumTest, sum_in_place) {
  auto out = makeVariable<double>(Dims{Dim::Y}, Shape{2});
  auto view = sum(var, Dim::X, out);

  const auto expected =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{3.0, 7.0});

  EXPECT_EQ(out, expected);
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST_F(SumTest, sum_in_place_bool) {
  auto out = makeVariable<int64_t>(Dims{Dim::Y}, Shape{2});
  auto view = sum(var_bool, Dim::X, out);

  const auto expected =
      makeVariable<int64_t>(Dims{Dim::Y}, Shape{2}, units::m, Values{1, 2});

  EXPECT_EQ(out, expected) << out << expected;
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST_F(SumTest, sum_in_place_bool_incorrect_out_type) {
  auto out = makeVariable<float>(Dims{Dim::Y}, Shape{2});
  // why this error? TypeError?
  EXPECT_THROW(sum(var_bool, Dim::X, out), except::UnitError);
}
