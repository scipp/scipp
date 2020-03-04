// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/core/event.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(EventTest, concatenate_variable) {
  auto a = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto a_ = a.sparseValues<double>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};
  auto b = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto b_ = b.sparseValues<double>();
  b_[0] = {1, 3};
  b_[1] = {};

  auto var = event::concatenate(a, b);
  EXPECT_TRUE(is_events(var));
  EXPECT_EQ(var.dims().volume(), 2);
  auto data = var.sparseValues<double>();
  EXPECT_TRUE(equals(data[0], {1, 2, 3, 1, 3}));
  EXPECT_TRUE(equals(data[1], {1, 2}));
}

TEST(EventTest, concatenate_variable_with_variances) {
  auto a = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2}, Values{},
                                            Variances{});
  auto a_vals = a.sparseValues<double>();
  a_vals[0] = {1, 2, 3};
  a_vals[1] = {1, 2};
  auto a_vars = a.sparseVariances<double>();
  a_vars[0] = {4, 5, 6};
  a_vars[1] = {4, 5};
  auto b = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2}, Values{},
                                            Variances{});
  auto b_vals = b.sparseValues<double>();
  b_vals[0] = {1, 3};
  b_vals[1] = {};
  auto b_vars = b.sparseVariances<double>();
  b_vars[0] = {7, 8};
  b_vars[1] = {};

  auto var = event::concatenate(a, b);
  EXPECT_TRUE(is_events(var));
  EXPECT_EQ(var.dims().volume(), 2);
  auto vals = var.sparseValues<double>();
  EXPECT_TRUE(equals(vals[0], {1, 2, 3, 1, 3}));
  EXPECT_TRUE(equals(vals[1], {1, 2}));
  auto vars = var.sparseVariances<double>();
  EXPECT_TRUE(equals(vars[0], {4, 5, 6, 7, 8}));
  EXPECT_TRUE(equals(vars[1], {4, 5}));
}
