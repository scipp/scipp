// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <limits>

#include "test_macros.h"

#include "scipp/dataset/event.h"
#include "scipp/variable/event.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(EventTest, concatenate_variable) {
  auto a = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto a_ = a.values<event_list<double>>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};
  auto b = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto b_ = b.values<event_list<double>>();
  b_[0] = {1, 3};
  b_[1] = {};

  auto var = variable::event::concatenate(a, b);
  EXPECT_TRUE(contains_events(var));
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

  auto var = variable::event::concatenate(a, b);
  EXPECT_TRUE(contains_events(var));
  EXPECT_EQ(var.dims().volume(), 2);
  auto vals = var.values<event_list<double>>();
  EXPECT_TRUE(equals(vals[0], {1, 2, 3, 1, 3}));
  EXPECT_TRUE(equals(vals[1], {1, 2}));
  auto vars = var.variances<event_list<double>>();
  EXPECT_TRUE(equals(vars[0], {4, 5, 6, 7, 8}));
  EXPECT_TRUE(equals(vars[1], {4, 5}));
}

struct EventConcatenateTest : public ::testing::Test {
  EventConcatenateTest() {
    eventsA = makeVariable<event_list<double>>(Dims{Dim::X}, Shape{2});
    auto a = eventsA.values<event_list<double>>();
    a[0] = {1, 2, 3};
    a[1] = {1, 2};
    eventsB = makeVariable<event_list<double>>(Dims{Dim::X}, Shape{2});
    auto b = eventsB.values<event_list<double>>();
    b[0] = {1, 3};
    b[1] = {};

    weightsA = makeVariable<event_list<double>>(
        Dims{Dim::X}, Shape{2}, units::counts, Values{}, Variances{});
    auto a_val = weightsA.values<event_list<double>>();
    a_val[0] = {1, 2, 3};
    a_val[1] = {1, 2};
    auto a_var = weightsA.variances<event_list<double>>();
    a_var[0] = {1, 2, 3};
    a_var[1] = {1, 2};
    weightsB = makeVariable<event_list<double>>(
        Dims{Dim::X}, Shape{2}, units::counts, Values{}, Variances{});
    auto b_val = weightsB.values<event_list<double>>();
    b_val[0] = {1, 3};
    b_val[1] = {};
    auto b_var = weightsB.variances<event_list<double>>();
    b_var[0] = {1, 3};
    b_var[1] = {};
  }
  Variable scalarA = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::counts,
                                          Values{1, 2}, Variances{3, 4});
  Variable scalarB = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::counts,
                                          Values{5, 6}, Variances{7, 8});
  Variable eventsA;
  Variable eventsB;
  Variable weightsA;
  Variable weightsB;
};

TEST_F(EventConcatenateTest, append_variable) {
  Variable var(eventsA);
  variable::event::append(var, eventsB);
  EXPECT_EQ(var, variable::event::concatenate(eventsA, eventsB));
}

TEST_F(EventConcatenateTest, data_array_identical_scalar_weights) {
  DataArray a(scalarA, {{Dim::Y, eventsA}});
  DataArray b(scalarA, {{Dim::Y, eventsB}});
  const auto result = event::concatenate(a, b);
  event::append(a, b);
  EXPECT_EQ(a, result);
  EXPECT_EQ(a.coords()[Dim::Y], variable::event::concatenate(eventsA, eventsB));
  EXPECT_EQ(a.data(), scalarA);
}

TEST_F(EventConcatenateTest, data_array_scalar_weights) {
  DataArray a(scalarA, {{Dim::Y, eventsA}});
  DataArray b(scalarB, {{Dim::Y, eventsB}});
  const auto result = event::concatenate(a, b);
  event::append(a, b);
  EXPECT_EQ(a, result);
  EXPECT_EQ(a.coords()[Dim::Y], variable::event::concatenate(eventsA, eventsB));
  EXPECT_EQ(a.data(), variable::event::concatenate(
                          variable::event::broadcast(scalarA, eventsA),
                          variable::event::broadcast(scalarB, eventsB)));
}

TEST_F(EventConcatenateTest, data_array_scalar_weights_a) {
  DataArray a(scalarA, {{Dim::Y, eventsA}});
  DataArray b(weightsB, {{Dim::Y, eventsB}});
  const auto result = event::concatenate(a, b);
  event::append(a, b);
  EXPECT_EQ(a, result);
  EXPECT_EQ(a.coords()[Dim::Y], variable::event::concatenate(eventsA, eventsB));
  EXPECT_EQ(a.data(),
            variable::event::concatenate(
                variable::event::broadcast(scalarA, eventsA), weightsB));
}

TEST_F(EventConcatenateTest, data_array_scalar_weights_b) {
  DataArray a(weightsA, {{Dim::Y, eventsA}});
  DataArray b(scalarB, {{Dim::Y, eventsB}});
  const auto result = event::concatenate(a, b);
  event::append(a, b);
  EXPECT_EQ(a, result);
  EXPECT_EQ(a.coords()[Dim::Y], variable::event::concatenate(eventsA, eventsB));
  EXPECT_EQ(a.data(),
            variable::event::concatenate(
                weightsA, variable::event::broadcast(scalarB, eventsB)));
}

TEST_F(EventConcatenateTest, data_array) {
  DataArray a(weightsA, {{Dim::Y, eventsA}});
  DataArray b(weightsB, {{Dim::Y, eventsB}});
  const auto result = event::concatenate(a, b);
  event::append(a, b);
  EXPECT_EQ(a, result);
  EXPECT_EQ(a.coords()[Dim::Y], variable::event::concatenate(eventsA, eventsB));
  EXPECT_EQ(a.data(), variable::event::concatenate(weightsA, weightsB));
}

struct EventBroadcastTest : public ::testing::Test {
  Variable shape = makeVariable<event_list<double>>(
      Dims{Dim::X}, Shape{2}, units::us,
      Values{event_list<double>(3), event_list<double>(1)});
  Variable dense = makeVariable<float>(Dims{Dim::X}, Shape{2}, units::counts,
                                       Values{1, 2}, Variances{3, 4});
  Variable expected = makeVariable<event_list<float>>(
      Dims{Dim::X}, Shape{2}, units::counts,
      Values{event_list<float>{1, 1, 1}, event_list<float>{2}},
      Variances{event_list<float>{3, 3, 3}, event_list<float>{4}});
};

TEST_F(EventBroadcastTest, variable) {
  EXPECT_EQ(variable::event::broadcast(dense, shape), expected);
}

TEST_F(EventBroadcastTest, data_array) {
  DataArray a(dense, {{Dim::Y, shape}});
  EXPECT_EQ(event::broadcast_weights(a), expected);
}

TEST_F(EventBroadcastTest, data_array_fail) {
  DataArray a(dense);
  EXPECT_THROW(event::broadcast_weights(a), except::EventDataError);
}

static auto make_events() {
  auto var = makeVariable<event_list<double>>(Dims{Dim::Z, Dim::Y}, units::us,
                                              Shape{3, 2});
  scipp::index count = 0;
  for (auto &v : var.values<event_list<double>>())
    v.resize(count++);
  return var;
}

static auto make_events_with_variances() {
  auto var = makeVariable<event_list<double>>(
      Dimensions{{Dim::Z, 3}, {Dim::Y, 2}}, Values{}, Variances{});
  scipp::index count = 0;
  for (auto &v : var.values<event_list<double>>())
    v.resize(count++);
  count = 0;
  for (auto &v : var.variances<event_list<double>>())
    v.resize(count++);
  return var;
}

TEST(EventSizesTest, fail_dense) {
  auto bad = makeVariable<double>(Values{1.0});
  EXPECT_ANY_THROW(variable::event::sizes(bad));
}

TEST(EventSizesTest, no_variances) {
  const auto var = make_events();
  auto expected = makeVariable<scipp::index>(Dims{Dim::Z, Dim::Y}, Shape{3, 2},
                                             Values{0, 1, 2, 3, 4, 5});
  EXPECT_EQ(variable::event::sizes(var), expected);
}

TEST(EventSizesTest, variances) {
  const auto var = make_events_with_variances();
  auto expected = makeVariable<scipp::index>(Dims{Dim::Z, Dim::Y}, Shape{3, 2},
                                             Values{0, 1, 2, 3, 4, 5});
  EXPECT_EQ(variable::event::sizes(var), expected);
}

struct EventFilterTest : public ::testing::Test {
  Variable data_with_variances = makeVariable<event_list<float>>(
      Dims{Dim::X}, Shape{2}, units::counts,
      Values{event_list<float>{1.1, 1.2, 1.3},
             event_list<float>{1.4, 1.5, 1.6, 1.7}},
      Variances{event_list<float>{1.1, 1.2, 1.3},
                event_list<float>{1.4, 1.5, 1.6, 1.7}});
  Variable data = makeVariable<event_list<float>>(
      Dims{Dim::X}, Shape{2}, units::counts,
      Values{event_list<float>{1.1, 1.2, 1.3},
             event_list<float>{1.4, 1.5, 1.6, 1.7}});
  Variable coord1 = makeVariable<event_list<float>>(
      Dims{Dim::X}, Shape{2}, units::us,
      Values{event_list<float>{3, 2, 1}, event_list<float>{2, 3, 4, 1}});
  Variable coord2 = makeVariable<event_list<int64_t>>(
      Dims{Dim::X}, Shape{2},
      Values{event_list<int64_t>{3, 2, 1}, event_list<int64_t>{2, 3, 4, 1}});
};

TEST_F(EventFilterTest, all) {
  const DataArray a(data, {{Dim::Y, coord1}});
  const auto interval =
      makeVariable<float>(Dims{Dim::Y}, Shape{2}, units::us, Values{0, 5});
  EXPECT_EQ(event::filter(a, Dim::Y, interval), a);
}

TEST_F(EventFilterTest, all_with_variances) {
  const DataArray a(data_with_variances, {{Dim::Y, coord1}});
  const auto interval =
      makeVariable<float>(Dims{Dim::Y}, Shape{2}, units::us, Values{0, 5});
  EXPECT_EQ(event::filter(a, Dim::Y, interval), a);
}

TEST_F(EventFilterTest, filter_1d_behavior_out_bounds) {
  // Filtering uses interval open on the right [left, right), just as histogram
  // events at 1,2,3 in first event list
  const DataArray a(data, {{Dim::Y, coord1}});
  Variable interval;
  DataArray filtered;

  interval =
      makeVariable<float>(Dims{Dim::Y}, Shape{2}, units::us, Values{0.0, 4.0});
  filtered = event::filter(a, Dim::Y, interval);
  EXPECT_EQ(scipp::size(filtered.values<event_list<float>>()[0]), 3);

  // left bound included
  interval =
      makeVariable<float>(Dims{Dim::Y}, Shape{2}, units::us, Values{1.0, 4.0});
  filtered = event::filter(a, Dim::Y, interval);
  EXPECT_EQ(scipp::size(filtered.values<event_list<float>>()[0]), 3);

  interval = makeVariable<float>(Dims{Dim::Y}, Shape{2}, units::us,
                                 Values{1.00001, 4.0});
  filtered = event::filter(a, Dim::Y, interval);
  EXPECT_EQ(scipp::size(filtered.values<event_list<float>>()[0]), 2);

  // right bound not included
  interval =
      makeVariable<float>(Dims{Dim::Y}, Shape{2}, units::us, Values{1.0, 3.0});
  filtered = event::filter(a, Dim::Y, interval);
  EXPECT_EQ(scipp::size(filtered.values<event_list<float>>()[0]), 2);
}

TEST_F(EventFilterTest, filter_1d) {
  const DataArray a(data, {{Dim::Y, coord1}, {Dim::Z, coord2}});
  const auto interval =
      makeVariable<float>(Dims{Dim::Y}, Shape{2}, units::us, Values{0.0, 2.5});

  const DataArray expected{
      makeVariable<event_list<float>>(
          Dims{Dim::X}, Shape{2}, units::counts,
          Values{event_list<float>{1.2, 1.3}, event_list<float>{1.4, 1.7}}),
      {{Dim::Y, makeVariable<event_list<float>>(
                    Dims{Dim::X}, Shape{2}, units::us,
                    Values{event_list<float>{2, 1}, event_list<float>{2, 1}})},
       {Dim::Z,
        makeVariable<event_list<int64_t>>(
            Dims{Dim::X}, Shape{2},
            Values{event_list<int64_t>{2, 1}, event_list<int64_t>{2, 1}})}}};

  EXPECT_EQ(event::filter(a, Dim::Y, interval), expected);
}

TEST_F(EventFilterTest, filter_1d_with_variances) {
  const DataArray a(data_with_variances, {{Dim::Y, coord1}, {Dim::Z, coord2}});
  const auto interval =
      makeVariable<float>(Dims{Dim::Y}, Shape{2}, units::us, Values{0.0, 2.5});

  const DataArray expected{
      makeVariable<event_list<float>>(
          Dims{Dim::X}, Shape{2}, units::counts,
          Values{event_list<float>{1.2, 1.3}, event_list<float>{1.4, 1.7}},
          Variances{event_list<float>{1.2, 1.3}, event_list<float>{1.4, 1.7}}),
      {{Dim::Y, makeVariable<event_list<float>>(
                    Dims{Dim::X}, Shape{2}, units::us,
                    Values{event_list<float>{2, 1}, event_list<float>{2, 1}})},
       {Dim::Z,
        makeVariable<event_list<int64_t>>(
            Dims{Dim::X}, Shape{2},
            Values{event_list<int64_t>{2, 1}, event_list<int64_t>{2, 1}})}}};

  EXPECT_EQ(event::filter(a, Dim::Y, interval), expected);
}

// Passes, but disabled since long running and using a lot of memory.
TEST_F(EventFilterTest, DISABLED_filter_1d_64bit_indices) {
  DataArray a(data, {{Dim::Y, coord1}});
  const auto interval =
      makeVariable<float>(Dims{Dim::Y}, Shape{2}, units::us, Values{1.0, 2.5});

  const scipp::index size = std::numeric_limits<int32_t>::max();
  auto &values = a.values<event_list<float>>()[0];
  values.clear();
  values.resize(size + 3);
  values[size + 0] = 1.1;
  values[size + 1] = 1.2;
  values[size + 2] = 1.3;
  auto &coord = a.coords()[Dim::Y].values<event_list<float>>()[0];
  coord.clear();
  coord.resize(size + 3);
  coord[size + 0] = 3;
  coord[size + 1] = 2;
  coord[size + 2] = 1;

  const DataArray expected{
      makeVariable<event_list<float>>(
          Dims{Dim::X}, Shape{2}, units::counts,
          Values{event_list<float>{1.2, 1.3}, event_list<float>{1.4, 1.7}}),
      {{Dim::Y,
        makeVariable<event_list<float>>(
            Dims{Dim::X}, Shape{2}, units::us,
            Values{event_list<float>{2, 1}, event_list<float>{2, 1}})}}};

  EXPECT_EQ(event::filter(a, Dim::Y, interval), expected);
}
