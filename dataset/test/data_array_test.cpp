// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/event.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/unaligned.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/operations.h"

#include "dataset_test_common.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(DataArrayTest, construct) {
  DatasetFactory3D factory;
  const auto dataset = factory.make();

  DataArray array(dataset["data_xyz"]);
  EXPECT_EQ(array, dataset["data_xyz"]);
  // Comparison ignores the name, so this is tested separately.
  EXPECT_EQ(array.name(), "data_xyz");
}

TEST(DataArrayTest, construct_fail) {
  // Invalid data
  EXPECT_THROW(DataArray(Variable{}), std::runtime_error);
  // Invalid unaligned data
  EXPECT_THROW(DataArray(UnalignedData{Dimensions{}, DataArray{}}),
               std::runtime_error);
}

TEST(DataArrayTest, setName) {
  DatasetFactory3D factory;
  const auto dataset = factory.make();
  DataArray array(dataset["data_xyz"]);

  array.setName("newname");
  EXPECT_EQ(array.name(), "newname");
}

TEST(DataArrayTest, sum_dataset_columns_via_DataArray) {
  DatasetFactory3D factory;
  auto dataset = factory.make();

  DataArray array(dataset["data_zyx"]);
  auto sum = array + dataset["data_xyz"];

  dataset["data_zyx"] += dataset["data_xyz"];

  // This would fail if the data items had attributes, since += preserves them
  // but + does not.
  EXPECT_EQ(sum, dataset["data_zyx"]);
}

TEST(DataArrayTest, fail_op_non_matching_coords) {
  auto coord_1 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  auto coord_2 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 4});
  DataArray da_1(data, {{Dim::X, coord_1}, {Dim::Y, data}});
  DataArray da_2(data, {{Dim::X, coord_2}, {Dim::Y, data}});
  // Fail because coordinates mismatched
  EXPECT_THROW(da_1 + da_2, except::CoordMismatchError);
  EXPECT_THROW(da_1 - da_2, except::CoordMismatchError);
}

auto make_events() {
  auto var = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  var.setUnit(units::us);
  auto vals = var.values<event_list<double>>();
  vals[0] = {1.1, 2.2, 3.3};
  vals[1] = {1.1, 2.2, 3.3, 5.5};
  return DataArray(makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::counts,
                                        Values{1, 1}, Variances{1, 1}),
                   {{Dim::X, var}});
}

auto make_histogram() {
  auto edges = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                    units::us, Values{0, 2, 4, 1, 3, 5});
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2.0, 3.0},
                                   Variances{0.3, 0.4});

  return DataArray(data, {{Dim::X, edges}});
}

auto make_histogram_no_variance() {
  auto edges = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                    units::us, Values{0, 2, 4, 1, 3, 5});
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2.0, 3.0});

  return DataArray(data, {{Dim::X, edges}});
}

TEST(DataArrayTest, astype) {
  DataArray a(
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}),
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{4, 5, 6})}});
  const auto x = astype(a, dtype<double>);
  EXPECT_EQ(x.data(),
            makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1., 2., 3.}));
}

TEST(DataArrayRealignedEventsArithmeticTest, fail_events_op_non_histogram) {
  const auto events = make_events();
  auto coord = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                    units::us, Values{0, 2, 1, 3});
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2.0, 3.0},
                                   Variances{0.3, 0.4});
  DataArray not_hist(data, {{Dim::X, coord}});

  // Fail due to coord mismatch between event coord and dense coord
  EXPECT_THROW(events * not_hist, except::CoordMismatchError);
  EXPECT_THROW(not_hist * events, except::CoordMismatchError);
  EXPECT_THROW(events / not_hist, except::CoordMismatchError);

  const auto realigned = unaligned::realign(
      events, {{Dim::X, Variable{not_hist.coords()[Dim::X]}}});

  // Fail because non-event operand has to be a histogram
  EXPECT_THROW(realigned * not_hist, except::BinEdgeError);
  EXPECT_THROW(not_hist * realigned, except::BinEdgeError);
  EXPECT_THROW(realigned / not_hist, except::BinEdgeError);
}

TEST(DataArrayRealignedEventsArithmeticTest, events_times_histogram) {
  const auto events = make_events();
  const auto hist = make_histogram();
  const auto realigned = unaligned::realign(
      DataArray(events), {{Dim::X, Variable{hist.coords()[Dim::X]}}});

  for (const auto &result : {realigned * hist, hist * realigned}) {
    EXPECT_EQ(result.coords(), realigned.coords());
    EXPECT_FALSE(result.hasData());
    EXPECT_TRUE(result.hasVariances());
    EXPECT_EQ(result.unit(), units::counts);

    const auto unaligned = result.unaligned();
    EXPECT_EQ(unaligned.coords()[Dim::X],
              realigned.unaligned().coords()[Dim::X]);
    const auto out_vals = unaligned.data().values<event_list<double>>();
    const auto out_vars = unaligned.data().variances<event_list<double>>();

    auto expected =
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 1, 1},
                             Variances{1, 1, 1}) *
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2.0, 3.0, 3.0},
                             Variances{0.3, 0.4, 0.4});
    EXPECT_TRUE(equals(out_vals[0], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[0], expected.variances<double>()));
    // out of range of edges -> dropped in realign step (independent of this op)
    expected =
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 1, 1},
                             Variances{1, 1, 1}) *
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2.0, 2.0, 3.0},
                             Variances{0.3, 0.3, 0.4});
    EXPECT_TRUE(equals(out_vals[1], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[1], expected.variances<double>()));
  }
  EXPECT_EQ(copy(realigned) *= hist, realigned * hist);
}

TEST(DataArrayRealignedEventsArithmeticTest,
     events_times_histogram_fail_too_many_realigned) {
  auto a = make_events();
  auto x = make_histogram();
  auto z(x);
  z.rename(Dim::X, Dim::Z);
  auto zx = z * x;
  using unaligned::realign;
  // Ok, one realigned dim but hist for multiple dims
  EXPECT_NO_THROW(realign(a, {{Dim::X, Variable{zx.coords()[Dim::X]}}}) * zx);
  a.coords().set(Dim::Z, a.coords()[Dim::X]);
  // Ok, `a` has multiple realigned dims, but hist is only for one of them
  EXPECT_NO_THROW(realign(a, {{Dim::X, Variable{x.coords()[Dim::X]}}}) * x);
  EXPECT_NO_THROW(realign(a, {{Dim::Z, Variable{z.coords()[Dim::Z]}}}) * z);
  // Multiple realigned dims and hist for multiple not implemented
  EXPECT_THROW(realign(a, {{Dim::X, Variable{zx.coords()[Dim::X]}},
                           {Dim::Z, Variable{zx.coords()[Dim::Z]}}}) *
                   zx,
               except::BinEdgeError);
}

TEST(DataArrayRealignedEventsArithmeticTest,
     events_times_histogram_without_variances) {
  const auto events = make_events();
  auto hist = make_histogram_no_variance();
  const auto realigned = unaligned::realign(
      DataArray(events), {{Dim::X, Variable{hist.coords()[Dim::X]}}});

  for (const auto &result : {realigned * hist, hist * realigned}) {
    EXPECT_EQ(result.coords(), realigned.coords());
    EXPECT_FALSE(result.hasData());
    EXPECT_TRUE(result.hasVariances());
    EXPECT_EQ(result.unit(), units::counts);

    const auto unaligned = result.unaligned();
    EXPECT_EQ(unaligned.coords()[Dim::X],
              realigned.unaligned().coords()[Dim::X]);
    const auto out_vals = unaligned.data().values<event_list<double>>();
    const auto out_vars = unaligned.data().variances<event_list<double>>();

    auto expected =
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 1, 1},
                             Variances{1, 1, 1}) *
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2.0, 3.0, 3.0});
    EXPECT_TRUE(equals(out_vals[0], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[0], expected.variances<double>()));
    // out of range of edges -> dropped in realign step (independent of this op)
    expected =
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 1, 1},
                             Variances{1, 1, 1}) *
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2.0, 2.0, 3.0});
    EXPECT_TRUE(equals(out_vals[1], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[1], expected.variances<double>()));
  }
  EXPECT_EQ(copy(realigned) *= hist, realigned * hist);
}

TEST(DataArrayRealignedEventsArithmeticTest,
     events_with_values_times_histogram) {
  auto events = make_events();
  const auto hist = make_histogram();
  Variable data(events.coords()[Dim::X]);
  data.setUnit(units::counts);
  data *= 0.0 * units::one;
  data += 2.0 * units::counts;
  events.setData(data);
  const auto realigned = unaligned::realign(
      DataArray(events), {{Dim::X, Variable{hist.coords()[Dim::X]}}});

  for (const auto &result : {realigned * hist, hist * realigned}) {
    EXPECT_EQ(result.coords(), realigned.coords());
    EXPECT_FALSE(result.hasData());
    EXPECT_TRUE(result.hasVariances());
    EXPECT_EQ(result.unit(), units::counts);

    const auto unaligned = result.unaligned();
    EXPECT_EQ(unaligned.coords()[Dim::X],
              realigned.unaligned().coords()[Dim::X]);
    const auto out_vals = unaligned.data().values<event_list<double>>();
    const auto out_vars = unaligned.data().variances<event_list<double>>();

    auto expected =
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2, 2, 2},
                             Variances{0, 0, 0}) *
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2.0, 3.0, 3.0},
                             Variances{0.3, 0.4, 0.4});
    EXPECT_TRUE(equals(out_vals[0], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[0], expected.variances<double>()));
    // out of range of edges -> value set to 0, consistent with rebin behavior
    // out of range of edges -> dropped in realign step (independent of this op)
    expected =
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2, 2, 2},
                             Variances{0, 0, 0}) *
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2.0, 2.0, 3.0},
                             Variances{0.3, 0.3, 0.4});
    EXPECT_TRUE(equals(out_vals[1], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[1], expected.variances<double>()));
  }
}

TEST(DataArrayRealignedEventsArithmeticTest, events_over_histogram) {
  const auto events = make_events();
  const auto hist = make_histogram();
  const auto realigned = unaligned::realign(
      DataArray(events), {{Dim::X, Variable{hist.coords()[Dim::X]}}});

  const auto result = realigned / hist;
  EXPECT_EQ(result.coords(), realigned.coords());
  EXPECT_FALSE(result.hasData());
  EXPECT_TRUE(result.hasVariances());
  EXPECT_EQ(result.unit(), units::counts);
  const auto unaligned = result.unaligned();
  EXPECT_EQ(unaligned.coords()[Dim::X], realigned.unaligned().coords()[Dim::X]);
  const auto out_vals = unaligned.data().values<event_list<double>>();
  const auto out_vars = unaligned.data().variances<event_list<double>>();

  auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 1, 1},
                           Variances{1, 1, 1}) /
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2.0, 3.0, 3.0},
                           Variances{0.3, 0.4, 0.4});
  EXPECT_TRUE(equals(out_vals[0], expected.values<double>()));
  EXPECT_TRUE(equals(out_vars[0], expected.variances<double>()));
  // out of range of edges -> dropped in realign step (independent of this op)
  expected = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 1, 1},
                                  Variances{1, 1, 1}) /
             makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2.0, 2.0, 3.0},
                                  Variances{0.3, 0.3, 0.4});
  EXPECT_TRUE(equals(out_vals[1], expected.values<double>()));
  EXPECT_TRUE(equals(span<const double>(out_vars[1]).subspan(0, 3),
                     expected.slice({Dim::X, 0, 3}).variances<double>()));

  auto result_inplace = copy(realigned);
  result_inplace /= hist;
  EXPECT_TRUE(is_approx(result_inplace.unaligned().data(),
                        result.unaligned().data(), 1e-16));
  EXPECT_EQ(result_inplace.coords(), result.coords());
  EXPECT_EQ(result_inplace.masks(), result.masks());
  EXPECT_EQ(result_inplace.attrs(), result.attrs());
}

struct DataArrayRealignedEventsPlusMinusTest : public ::testing::Test {
protected:
  DataArrayRealignedEventsPlusMinusTest() {
    eventsB = eventsA;
    eventsB.coords()[Dim::X] += 0.01 * units::us;
    event::append(eventsB, eventsA);
    eventsB.coords()[Dim::X] += 0.02 * units::us;
    a = unaligned::realign(eventsA, {{Dim::X, edges}});
    b = unaligned::realign(eventsB, {{Dim::X, edges}});
  }

  DataArray eventsA = make_events();
  DataArray eventsB;
  Variable edges = makeVariable<double>(Dims{Dim::X}, Shape{4}, units::us,
                                        Values{0, 2, 4, 6});
  DataArray a;
  DataArray b;
};

TEST_F(DataArrayRealignedEventsPlusMinusTest, plus) {
  EXPECT_EQ(histogram(a + b), histogram(a) + histogram(b));
}

TEST_F(DataArrayRealignedEventsPlusMinusTest, minus) {
  EXPECT_EQ(histogram(a - b), histogram(a) - histogram(b));
}

TEST_F(DataArrayRealignedEventsPlusMinusTest, plus_equals) {
  auto out(a);
  out += b;
  EXPECT_EQ(out, a + b);
  out -= b;
  EXPECT_NE(out, a); // events not removed by "undo" of addition
  EXPECT_NE(histogram(out), histogram(a)); // mismatching variances
  EXPECT_EQ(out, a + b - b);
}

TEST_F(DataArrayRealignedEventsPlusMinusTest, plus_equals_self) {
  auto out(a);
  out += out;
  EXPECT_EQ(out, a + a);
}

TEST_F(DataArrayRealignedEventsPlusMinusTest, minus_equals) {
  auto out(a);
  out -= b;
  EXPECT_EQ(out, a - b);
}

TEST_F(DataArrayRealignedEventsPlusMinusTest, minus_equals_self) {
  auto out(a);
#pragma clang diagnostic ignored "-Wself-assign"
  out -= out;
  EXPECT_EQ(out, a - a);
}

TEST_F(DataArrayRealignedEventsPlusMinusTest, plus_nonscalar_weights) {
  auto c = a - b; // subtraction yields nonscalar weights
  EXPECT_EQ(histogram(c + a), histogram(a) - histogram(b) + histogram(a));
  EXPECT_EQ(histogram(c + a), histogram(a + c));
  EXPECT_EQ(histogram(c + c),
            histogram(a) - histogram(b) + histogram(a) - histogram(b));
}

TEST_F(DataArrayRealignedEventsPlusMinusTest, minus_nonscalar_weights) {
  auto c = a - b; // subtraction yields nonscalar weights
  EXPECT_EQ(histogram(c - a), histogram(a) - histogram(b) - histogram(a));
  EXPECT_EQ(histogram(a - c), histogram(a) + histogram(b) - histogram(a));
  EXPECT_EQ(histogram(c - c),
            histogram(a) - histogram(b) - histogram(a) + histogram(b));
}
