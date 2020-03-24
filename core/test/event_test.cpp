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
  auto a_ = a.values<event_list<double>>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};
  auto b = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto b_ = b.values<event_list<double>>();
  b_[0] = {1, 3};
  b_[1] = {};

  auto var = event::concatenate(a, b);
  EXPECT_TRUE(is_events(var));
  EXPECT_EQ(var.dims().volume(), 2);
  auto data = var.values<event_list<double>>();
  EXPECT_TRUE(equals(data[0], {1, 2, 3, 1, 3}));
  EXPECT_TRUE(equals(data[1], {1, 2}));
}

TEST(EventTest, concatenate_variable_with_variances) {
  auto a = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2}, Values{},
                                            Variances{});
  auto a_vals = a.values<event_list<double>>();
  a_vals[0] = {1, 2, 3};
  a_vals[1] = {1, 2};
  auto a_vars = a.variances<event_list<double>>();
  a_vars[0] = {4, 5, 6};
  a_vars[1] = {4, 5};
  auto b = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2}, Values{},
                                            Variances{});
  auto b_vals = b.values<event_list<double>>();
  b_vals[0] = {1, 3};
  b_vals[1] = {};
  auto b_vars = b.variances<event_list<double>>();
  b_vars[0] = {7, 8};
  b_vars[1] = {};

  auto var = event::concatenate(a, b);
  EXPECT_TRUE(is_events(var));
  EXPECT_EQ(var.dims().volume(), 2);
  auto vals = var.values<event_list<double>>();
  EXPECT_TRUE(equals(vals[0], {1, 2, 3, 1, 3}));
  EXPECT_TRUE(equals(vals[1], {1, 2}));
  auto vars = var.variances<event_list<double>>();
  EXPECT_TRUE(equals(vars[0], {4, 5, 6, 7, 8}));
  EXPECT_TRUE(equals(vars[1], {4, 5}));
}

struct EventBroadcastTest : public ::testing::Test {
  Variable shape = makeVariable<event_list<double>>(
      Dims{Dim::X}, Shape{2}, units::Unit(units::us),
      Values{event_list<double>(3), event_list<double>(1)});
  Variable dense =
      makeVariable<float>(Dims{Dim::X}, Shape{2}, units::Unit(units::counts),
                          Values{1, 2}, Variances{3, 4});
  Variable expected = makeVariable<event_list<float>>(
      Dims{Dim::X}, Shape{2}, units::Unit(units::counts),
      Values{event_list<float>{1, 1, 1}, event_list<float>{2}},
      Variances{event_list<float>{3, 3, 3}, event_list<float>{4}});
};

TEST_F(EventBroadcastTest, variable) {
  EXPECT_EQ(event::broadcast(dense, shape), expected);
}

TEST_F(EventBroadcastTest, data_array) {
  DataArray a(dense, {{Dim::Y, shape}});
  EXPECT_EQ(event::broadcast_weights(a), expected);
}

TEST_F(EventBroadcastTest, data_array_fail) {
  DataArray a(dense);
  EXPECT_THROW(event::broadcast_weights(a), except::EventDataError);
}
