// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/except.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/variable.h"

using namespace scipp;

TEST(ReduceTest, min_max_fails) {
  const auto bad = makeVariable<double>(Dims{Dim::X}, Shape{2});
  EXPECT_THROW(static_cast<void>(min(bad, Dim::Y)), except::DimensionError);
  EXPECT_THROW(static_cast<void>(max(bad, Dim::Y)), except::DimensionError);
}

TEST(ReduceTest, min_max) {
  const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                        Values{1, 2, 3, 4});
  EXPECT_EQ(max(var, Dim::X),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{3, 4}));
  EXPECT_EQ(max(var, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 4}));
  EXPECT_EQ(min(var, Dim::X),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 2}));
  EXPECT_EQ(min(var, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 3}));
}

TEST(ReduceTest, min_max_with_variances) {
  const auto var =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                           Values{1, 2, 3, 4}, Variances{5, 6, 7, 8});
  EXPECT_EQ(max(var, Dim::X),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{3, 4},
                                 Variances{7, 8}));
  EXPECT_EQ(max(var, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 4},
                                 Variances{6, 8}));
  EXPECT_EQ(min(var, Dim::X),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 2},
                                 Variances{5, 6}));
  EXPECT_EQ(min(var, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 3},
                                 Variances{5, 7}));
}

TEST(ReduceTest, min_max_all_dims) {
  const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                        Values{1, 2, 3, 4});
  EXPECT_EQ(min(var), makeVariable<double>(Values{1}));
  EXPECT_EQ(max(var), makeVariable<double>(Values{4}));
  EXPECT_EQ(min(min(var)), min(var));
  EXPECT_EQ(max(min(var)), min(var));
}

TEST(ReduceTest, all_any_all_dims) {
  const auto var = makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                      Values{true, false, false, false});
  EXPECT_EQ(all(var), makeVariable<bool>(Values{false}));
  EXPECT_EQ(any(var), makeVariable<bool>(Values{true}));
  EXPECT_EQ(all(all(var)), all(var));
  EXPECT_EQ(any(all(var)), all(var));
  EXPECT_EQ(all(any(var)), any(var));
  EXPECT_EQ(any(any(var)), any(var));
}
