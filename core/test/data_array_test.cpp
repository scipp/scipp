// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/comparison.h"
#include "scipp/core/dataset.h"
#include "scipp/core/event.h"
#include "scipp/core/histogram.h"
#include "scipp/core/unaligned.h"

#include "dataset_test_common.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::core;

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

TEST(DataArrayTest, reciprocal) {
  DatasetFactory3D factory;
  const auto dataset = factory.make();
  DataArray array(dataset["data_zyx"]);
  EXPECT_EQ(reciprocal(array).data(), reciprocal(array.data()));
}

auto make_sparse() {
  auto var = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  var.setUnit(units::us);
  auto vals = var.values<event_list<double>>();
  vals[0] = {1.1, 2.2, 3.3};
  vals[1] = {1.1, 2.2, 3.3, 5.5};
  return DataArray(makeVariable<double>(Dims{Dim::Y}, Shape{2},
                                        units::Unit(units::counts),
                                        Values{1, 1}, Variances{1, 1}),
                   {{Dim::X, var}});
}

auto make_histogram() {
  // 2D edges on realigned not supported!?
  auto edges =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                           units::Unit(units::us), Values{0, 2, 4, 1, 3, 5});
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2.0, 3.0},
                                   Variances{0.3, 0.4});

  return DataArray(data, {{Dim::X, edges}});
}

auto make_histogram_no_variance() {
  auto edges =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                           units::Unit(units::us), Values{0, 2, 4, 1, 3, 5});
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2.0, 3.0});

  return DataArray(data, {{Dim::X, edges}});
}

TEST(DataArrayTest, astype) {
  DataArray a(
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}),
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{4, 5, 6})}});
  const auto x = astype(a, DType::Double);
  EXPECT_EQ(x.data(),
            makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1., 2., 3.}));
}

TEST(DataArrayRealignedEventsArithmeticTest, fail_sparse_op_non_histogram) {
  const auto sparse = make_sparse();
  auto coord = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                    Values{0, 2, 1, 3});
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2.0, 3.0},
                                   Variances{0.3, 0.4});
  DataArray not_hist(data, {{Dim::X, coord}});

  EXPECT_THROW(sparse * not_hist, except::VariableMismatchError);
  EXPECT_THROW(not_hist * sparse, except::VariableMismatchError);
  EXPECT_THROW(sparse / not_hist, except::VariableMismatchError);
}

TEST(DataArrayRealignedEventsArithmeticTest, events_times_histogram) {
  const auto sparse = make_sparse();
  const auto hist = make_histogram();
  const auto realigned = unaligned::realign(
      DataArray(sparse), {{Dim::X, Variable{hist.coords()[Dim::X]}}});

  for (const auto result : {realigned * hist, hist * realigned}) {
    EXPECT_EQ(result.coords(), realigned.coords());
    EXPECT_FALSE(result.hasData());
    EXPECT_TRUE(result.hasVariances());
    EXPECT_EQ(result.unit(), units::counts);

    const auto unaligned = result.unaligned();
    EXPECT_EQ(unaligned.coords()[Dim::X], sparse.coords()[Dim::X]);
    const auto out_vals = unaligned.data().values<event_list<double>>();
    const auto out_vars = unaligned.data().variances<event_list<double>>();

    auto expected =
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 1, 1},
                             Variances{1, 1, 1}) *
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2.0, 3.0, 3.0},
                             Variances{0.3, 0.4, 0.4});
    EXPECT_TRUE(equals(out_vals[0], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[0], expected.variances<double>()));
    // out of range of edges -> value set to 0, consistent with rebin behavior
    expected =
        makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 1, 1, 1},
                             Variances{1, 1, 1, 1}) *
        makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{2.0, 2.0, 3.0, 0.0},
                             Variances{0.3, 0.3, 0.4, 0.0});
    EXPECT_TRUE(equals(out_vals[1], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[1], expected.variances<double>()));
  }
  EXPECT_EQ(copy(realigned) *= hist, realigned * hist);
}

TEST(DataArrayRealignedEventsArithmeticTest,
     events_times_histogram_without_variances) {
  const auto sparse = make_sparse();
  auto hist = make_histogram_no_variance();
  const auto realigned = unaligned::realign(
      DataArray(sparse), {{Dim::X, Variable{hist.coords()[Dim::X]}}});

  for (const auto result : {realigned * hist, hist * realigned}) {
    EXPECT_EQ(result.coords(), realigned.coords());
    EXPECT_FALSE(result.hasData());
    EXPECT_TRUE(result.hasVariances());
    EXPECT_EQ(result.unit(), units::counts);

    const auto unaligned = result.unaligned();
    EXPECT_EQ(unaligned.coords()[Dim::X], sparse.coords()[Dim::X]);
    const auto out_vals = unaligned.data().values<event_list<double>>();
    const auto out_vars = unaligned.data().variances<event_list<double>>();

    auto expected =
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 1, 1},
                             Variances{1, 1, 1}) *
        makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2.0, 3.0, 3.0});
    EXPECT_TRUE(equals(out_vals[0], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[0], expected.variances<double>()));
    // out of range of edges -> value set to 0, consistent with rebin behavior
    expected = makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 1, 1, 1},
                                    Variances{1, 1, 1, 1}) *
               makeVariable<double>(Dims{Dim::X}, Shape{4},
                                    Values{2.0, 2.0, 3.0, 0.0});
    EXPECT_TRUE(equals(out_vals[1], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[1], expected.variances<double>()));
  }
  EXPECT_EQ(copy(realigned) *= hist, realigned * hist);
}

TEST(DataArrayRealignedEventsArithmeticTest,
     events_with_values_times_histogram) {
  auto sparse = make_sparse();
  const auto hist = make_histogram();
  Variable data(sparse.coords()[Dim::X]);
  data.setUnit(units::counts);
  data *= 0.0;
  data += 2.0 * units::Unit(units::counts);
  sparse.setData(data);
  const auto realigned = unaligned::realign(
      DataArray(sparse), {{Dim::X, Variable{hist.coords()[Dim::X]}}});

  for (const auto result : {realigned * hist, hist * realigned}) {
    EXPECT_EQ(result.coords(), realigned.coords());
    EXPECT_FALSE(result.hasData());
    EXPECT_TRUE(result.hasVariances());
    EXPECT_EQ(result.unit(), units::counts);

    const auto unaligned = result.unaligned();
    EXPECT_EQ(unaligned.coords()[Dim::X], sparse.coords()[Dim::X]);
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
    expected =
        makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{2, 2, 2, 2},
                             Variances{0, 0, 0, 0}) *
        makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{2.0, 2.0, 3.0, 0.0},
                             Variances{0.3, 0.3, 0.4, 0.0});
    EXPECT_TRUE(equals(out_vals[1], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[1], expected.variances<double>()));
  }
}

TEST(DataArrayRealignedEventsArithmeticTest, events_over_histogram) {
  const auto sparse = make_sparse();
  const auto hist = make_histogram();
  const auto realigned = unaligned::realign(
      DataArray(sparse), {{Dim::X, Variable{hist.coords()[Dim::X]}}});

  const auto result = realigned / hist;
  EXPECT_EQ(result.coords(), realigned.coords());
  EXPECT_FALSE(result.hasData());
  EXPECT_TRUE(result.hasVariances());
  EXPECT_EQ(result.unit(), units::counts);
  const auto unaligned = result.unaligned();
  EXPECT_EQ(unaligned.coords()[Dim::X], sparse.coords()[Dim::X]);
  const auto out_vals = unaligned.data().values<event_list<double>>();
  const auto out_vars = unaligned.data().variances<event_list<double>>();

  auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 1, 1},
                           Variances{1, 1, 1}) /
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{2.0, 3.0, 3.0},
                           Variances{0.3, 0.4, 0.4});
  EXPECT_TRUE(equals(out_vals[0], expected.values<double>()));
  EXPECT_TRUE(equals(out_vars[0], expected.variances<double>()));
  expected =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 1, 1, 1},
                           Variances{1, 1, 1, 1}) /
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{2.0, 2.0, 3.0, 0.0},
                           Variances{0.3, 0.3, 0.4, 0.0});
  EXPECT_TRUE(equals(out_vals[1], expected.values<double>()));
  EXPECT_TRUE(equals(span<const double>(out_vars[1]).subspan(0, 3),
                     expected.slice({Dim::X, 0, 3}).variances<double>()));
  EXPECT_TRUE(std::isnan(out_vars[1][3]));

  auto result_inplace = copy(sparse);
  result_inplace /= hist;
  EXPECT_TRUE(is_approx(result_inplace.data(), result.data(), 1e-16));
  EXPECT_EQ(result_inplace.coords(), result.coords());
  EXPECT_EQ(result_inplace.masks(), result.masks());
  EXPECT_EQ(result_inplace.attrs(), result.attrs());
}

struct DataArrayRealignedEventsPlusMinusTest : public ::testing::Test {
protected:
  DataArrayRealignedEventsPlusMinusTest() {
    eventsB = eventsA;
    eventsB.coords()[Dim::X] += 0.01 * units::Unit(units::us);
    event::append(eventsB, eventsA);
    eventsB.coords()[Dim::X] += 0.02 * units::Unit(units::us);
    a = unaligned::realign(eventsA, {{Dim::X, edges}});
    b = unaligned::realign(eventsB, {{Dim::X, edges}});
  }

  DataArray eventsA = make_sparse();
  DataArray eventsB;
  Variable edges = makeVariable<double>(
      Dims{Dim::X}, Shape{4}, units::Unit(units::us), Values{0, 2, 4, 6});
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

TEST_F(DataArrayRealignedEventsPlusMinusTest, minus_equals) {
  auto out(a);
  out -= b;
  EXPECT_EQ(out, a - b);
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
