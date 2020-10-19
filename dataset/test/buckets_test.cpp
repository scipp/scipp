// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bucket.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::dataset;

class DataArrayBucketTest : public ::testing::Test {
protected:
  using Model = variable::DataModel<bucket<DataArray>>;
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable data =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  Variable var{std::make_unique<Model>(indices, Dim::X, buffer)};
};

TEST_F(DataArrayBucketTest, concatenate) {
  const auto result = buckets::concatenate(var, var * (3.0 * units::one));
  Variable out_indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 4}, std::pair{4, 8}});
  Variable out_data = makeVariable<double>(Dims{Dim::X}, Shape{8},
                                           Values{1, 2, 3, 6, 3, 4, 9, 12});
  Variable out_x = makeVariable<double>(Dims{Dim::X}, Shape{8},
                                        Values{2, 4, 2, 4, 6, 8, 6, 8});
  DataArray out_buffer = DataArray(out_data, {{Dim::X, out_x}});
  EXPECT_EQ(result,
            Variable(std::make_unique<Model>(out_indices, Dim::X, out_buffer)));

  // "in-place" append gives same as concatenate
  buckets::append(var, var * (3.0 * units::one));
  EXPECT_EQ(result, var);
  buckets::append(var, -var);
}

TEST_F(DataArrayBucketTest, concatenate_with_broadcast) {
  auto var2 = var;
  var2.rename(Dim::Y, Dim::Z);
  var2 *= 3.0 * units::one;
  const auto result = buckets::concatenate(var, var2);
  Variable out_indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::Y, Dim::Z}, Shape{2, 2},
      Values{std::pair{0, 4}, std::pair{4, 8}, std::pair{8, 12},
             std::pair{12, 16}});
  Variable out_data = makeVariable<double>(
      Dims{Dim::X}, Shape{16},
      Values{1, 2, 3, 6, 1, 2, 9, 12, 3, 4, 3, 6, 3, 4, 9, 12});
  Variable out_x = makeVariable<double>(
      Dims{Dim::X}, Shape{16},
      Values{2, 4, 2, 4, 2, 4, 6, 8, 6, 8, 2, 4, 6, 8, 6, 8});
  DataArray out_buffer = DataArray(out_data, {{Dim::X, out_x}});
  EXPECT_EQ(result,
            Variable(std::make_unique<Model>(out_indices, Dim::X, out_buffer)));

  // Broadcast not possible for in-place append
  EXPECT_THROW(buckets::append(var, var2), except::DimensionMismatchError);
}

TEST_F(DataArrayBucketTest, histogram) {
  Variable weights = makeVariable<double>(
      Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4}, Variances{1, 2, 3, 4});
  DataArray events = DataArray(weights, {{Dim::Z, data}});
  Variable buckets{std::make_unique<Model>(indices, Dim::X, events)};
  // `buckets` *does not* depend on the histogramming dimension
  const auto bin_edges =
      makeVariable<double>(Dims{Dim::Z}, Shape{4}, Values{0, 1, 2, 4});
  EXPECT_EQ(buckets::histogram(buckets, bin_edges),
            makeVariable<double>(Dims{Dim::Y, Dim::Z}, Shape{2, 3},
                                 Values{0, 1, 2, 0, 0, 3},
                                 Variances{0, 1, 2, 0, 0, 3}));
}

TEST_F(DataArrayBucketTest, histogram_existing_dim) {
  Variable weights = makeVariable<double>(
      Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4}, Variances{1, 2, 3, 4});
  DataArray events = DataArray(weights, {{Dim::Y, data}});
  Variable buckets{std::make_unique<Model>(indices, Dim::X, events)};
  // `buckets` *does* depend on the histogramming dimension
  const auto bin_edges =
      makeVariable<double>(Dims{Dim::Y}, Shape{4}, Values{0, 1, 2, 4});
  const auto expected = makeVariable<double>(
      Dims{Dim::Y}, Shape{3}, Values{0, 1, 5}, Variances{0, 1, 5});
  EXPECT_EQ(buckets::histogram(buckets, bin_edges), expected);

  // Histogram data array containing bucket variable
  DataArray a(buckets);
  EXPECT_EQ(histogram(a, bin_edges),
            DataArray(expected, {{Dim::Y, bin_edges}}));
  // Masked data array
  a.masks().set(
      "mask", makeVariable<bool>(Dims{Dim::Y}, Shape{2}, Values{false, true}));
  EXPECT_EQ(histogram(a, bin_edges),
            DataArray(makeVariable<double>(Dims{Dim::Y}, Shape{3},
                                           Values{0, 1, 2}, Variances{0, 1, 2}),
                      {{Dim::Y, bin_edges}}));
}

TEST_F(DataArrayBucketTest, sum) {
  EXPECT_EQ(buckets::sum(var),
            makeVariable<double>(indices.dims(), Values{3, 7}));
}

class DataArrayBucketMapTest : public ::testing::Test {
protected:
  using ModelVariable = variable::DataModel<bucket<Variable>>;
  using ModelDataArray = variable::DataModel<bucket<DataArray>>;
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable data =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  Variable weights = makeVariable<double>(
      Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4}, Variances{1, 2, 3, 4});
  DataArray events = DataArray(weights, {{Dim::Z, data}});
  Variable buckets{std::make_unique<ModelDataArray>(indices, Dim::X, events)};
  // `buckets` *does not* depend on the histogramming dimension
  Variable bin_edges =
      makeVariable<double>(Dims{Dim::Z}, Shape{4}, Values{0, 1, 2, 4});
  DataArray histogram = DataArray(Variable(bin_edges.slice({Dim::Z, 1, 4})),
                                  {{Dim::Z, bin_edges}});
};

TEST_F(DataArrayBucketMapTest, map) {
  const auto out = buckets::map(histogram, buckets, Dim::Z);
  // event coords 1,2,3,4
  // histogram:
  // | 1 | 2 | 4 |
  // 0   1   2   4
  const auto expected_scale =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{2, 4, 4, 0});
  EXPECT_EQ(out, Variable(std::make_unique<ModelVariable>(indices, Dim::X,
                                                          expected_scale)));

  // Mapping result can be used to scale
  auto scaled = buckets * out;
  const auto expected = Variable{std::make_unique<ModelDataArray>(
      indices, Dim::X, events * expected_scale)};
  EXPECT_EQ(scaled, expected);

  // Mapping and scaling also works for slices
  auto partial = buckets;
  for (auto s : {Slice(Dim::Y, 0), Slice(Dim::Y, 1)})
    partial.slice(s) *= buckets::map(histogram, buckets.slice(s), Dim::Z);
  EXPECT_EQ(partial, expected);
}

TEST_F(DataArrayBucketMapTest, map_masked) {
  histogram.masks().set(
      "mask", makeVariable<bool>(histogram.dims(), Values{false, true, false}));
  const auto out = buckets::map(histogram, buckets, Dim::Z);
  const auto expected_scale =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{0, 4, 4, 0});
  EXPECT_EQ(out, Variable(std::make_unique<ModelVariable>(indices, Dim::X,
                                                          expected_scale)));
}

class DataArrayBucketScaleTest : public ::testing::Test {
protected:
  using Model = variable::DataModel<bucket<DataArray>>;
  auto make_indices() const {
    return makeVariable<std::pair<scipp::index, scipp::index>>(
        Dims{Dim::Y, Dim::X}, Shape{2, 1},
        Values{std::pair{0, 3}, std::pair{3, 7}});
  }
  auto make_events() const {
    auto weights = makeVariable<double>(Dims{Dim("event")}, Shape{7}, units::us,
                                        Values{1, 2, 1, 3, 1, 1, 1},
                                        Variances{1, 3, 1, 2, 1, 1, 1});
    auto coord =
        makeVariable<double>(Dims{Dim("event")}, Shape{7}, units::us,
                             Values{1.1, 2.2, 3.3, 1.1, 2.2, 3.3, 5.5});
    return DataArray(weights, {{Dim::X, coord}});
  }

  auto make_histogram() const {
    auto edges = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                      units::us, Values{0, 2, 4, 1, 3, 5});
    auto data = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                     Values{2.0, 3.0, 2.0, 3.0},
                                     Variances{0.3, 0.4, 0.3, 0.4});
    return DataArray(data, {{Dim::X, edges}});
  }

  auto make_histogram_no_variance() const {
    auto edges = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                      units::us, Values{0, 2, 4, 1, 3, 5});
    auto data = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                     Values{2.0, 3.0, 2.0, 3.0});
    return DataArray(data, {{Dim::X, edges}});
  }

  auto make_buckets(const DataArray &events,
                    const std::map<Dim, VariableConstView> coords = {}) const {
    auto array = DataArray(Variable(
        std::make_unique<Model>(make_indices(), Dim("event"), events)));
    for (const auto &[dim, coord] : coords)
      array.coords().set(dim, coord);
    return array;
  }
};

TEST_F(DataArrayBucketScaleTest, fail_events_op_non_histogram) {
  const auto events = make_events();
  auto coord = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                    units::us, Values{0, 2, 1, 3});
  auto data = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                   Values{2.0, 3.0, 2.0, 3.0},
                                   Variances{0.3, 0.4, 0.3, 0.4});
  DataArray not_hist(data, {{Dim::X, coord}});

  // Fail due to coord mismatch between event coord and dense coord
  EXPECT_THROW(events * not_hist, except::CoordMismatchError);
  EXPECT_THROW(not_hist * events, except::CoordMismatchError);
  EXPECT_THROW(events / not_hist, except::CoordMismatchError);

  auto buckets = make_buckets(events);

  // Fail because non-event operand has to be a histogram
  EXPECT_THROW(buckets::scale(buckets, not_hist), except::BinEdgeError);
  // We have a single bucket in X, so setting the "same" coord as in `not_hist`
  // we have a matching coord, it it would be a bin-edge coord on `buckets`.
  buckets.coords().set(Dim::X, not_hist.coords()[Dim::X]);
  EXPECT_THROW(buckets::scale(buckets, not_hist), except::BinEdgeError);
}

TEST_F(DataArrayBucketScaleTest, events_times_histogram) {
  const auto events = make_events();
  const auto hist = make_histogram();
  auto buckets = make_buckets(events);
  buckets::scale(buckets, hist);

  auto expected_weights = makeVariable<double>(
      Dims{Dim("event")}, Shape{7}, units::us, Values{1, 2, 1, 3, 1, 1, 1},
      Variances{1, 3, 1, 2, 1, 1, 1});
  // Last event is out of bounds and scaled to 0.0
  expected_weights *= makeVariable<double>(
      Dims{Dim("event")}, Shape{7}, Values{2.0, 3.0, 3.0, 2.0, 2.0, 3.0, 0.0},
      Variances{0.3, 0.4, 0.4, 0.3, 0.3, 0.4, 0.0});
  auto expected_events = events;
  expected_events.data().assign(expected_weights);

  EXPECT_EQ(buckets, make_buckets(expected_events));
}

TEST_F(DataArrayBucketScaleTest,
       events_times_histogram_fail_too_many_bucketed_dims) {
  auto x = make_histogram();
  auto z(x);
  z.rename(Dim::X, Dim::Z);
  auto zx = z * x;
  auto events = make_events();
  events.coords().set(Dim::Z, events.coords()[Dim::X]);
  auto buckets = make_buckets(events);
  // Ok, `buckets` has multiple bucketed dims, but hist is only for one of them
  EXPECT_NO_THROW(buckets::scale(buckets, x));
  EXPECT_NO_THROW(buckets::scale(buckets, z));
  // Multiple realigned dims and hist for multiple not implemented
  EXPECT_THROW(buckets::scale(buckets, zx), except::BinEdgeError);
}

class DataArrayBucketPlusMinusTest : public ::testing::Test {
protected:
  auto make_events() const {
    auto weights = makeVariable<double>(
        Dims{Dim("event")}, Shape{7}, units::counts,
        Values{1, 2, 1, 3, 1, 1, 1}, Variances{1, 3, 1, 2, 1, 1, 1});
    auto coord =
        makeVariable<double>(Dims{Dim("event")}, Shape{7}, units::us,
                             Values{1.1, 2.2, 3.3, 1.1, 2.2, 3.3, 5.5});
    return DataArray(weights, {{Dim::X, coord}});
  }

  DataArrayBucketPlusMinusTest() {
    eventsA = make_events();
    eventsB = eventsA;
    eventsB.coords()[Dim::X] += 0.01 * units::us;
    eventsB = concatenate(eventsB, eventsA, Dim("event"));
    eventsB.coords()[Dim::X] += 0.02 * units::us;
    using Model = variable::DataModel<bucket<DataArray>>;
    a = DataArray(Variable(std::make_unique<Model>(
        makeVariable<std::pair<scipp::index, scipp::index>>(
            Dims{Dim::Y, Dim::X}, Shape{2, 1},
            Values{std::pair{0, 3}, std::pair{3, 7}}),
        Dim("event"), eventsA)));
    b = DataArray(Variable(std::make_unique<Model>(
        makeVariable<std::pair<scipp::index, scipp::index>>(
            Dims{Dim::Y, Dim::X}, Shape{2, 1},
            Values{std::pair{0, 5}, std::pair{5, 14}}),
        Dim("event"), eventsB)));
  }

  DataArray eventsA;
  DataArray eventsB;
  Variable edges = makeVariable<double>(Dims{Dim::X}, Shape{4}, units::us,
                                        Values{0, 2, 4, 6});
  DataArray a;
  DataArray b;
};

TEST_F(DataArrayBucketPlusMinusTest, plus) {
  using buckets::sum;
  EXPECT_EQ(sum(buckets::concatenate(a, b)), sum(a) + sum(b));
}

TEST_F(DataArrayBucketPlusMinusTest, minus) {
  using buckets::sum;
  auto tmp = -b;
  EXPECT_EQ(b.unit(), units::one);
  EXPECT_EQ(tmp.unit(), units::one);
  EXPECT_EQ(sum(buckets::concatenate(a, -b)), sum(a) - sum(b));
}

TEST_F(DataArrayBucketPlusMinusTest, plus_equals) {
  auto out(a);
  buckets::append(out, b);
  EXPECT_EQ(out, buckets::concatenate(a, b));
  buckets::append(out, -b);
  EXPECT_NE(out, a); // events not removed by "undo" of addition
  EXPECT_NE(buckets::sum(out), buckets::sum(a)); // mismatching variances
  EXPECT_EQ(out, buckets::concatenate(buckets::concatenate(a, b), -b));
}

TEST_F(DataArrayBucketPlusMinusTest, plus_equals_self) {
  auto out(a);
  buckets::append(out, out);
  EXPECT_EQ(out, buckets::concatenate(a, a));
}

TEST_F(DataArrayBucketPlusMinusTest, minus_equals) {
  auto out(a);
  buckets::append(out, -b);
  EXPECT_EQ(out, buckets::concatenate(a, -b));
}

class DatasetBucketTest : public ::testing::Test {
protected:
  using Model = variable::DataModel<bucket<Dataset>>;
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 3}});
  Variable column =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  Dataset buffer0;
  Dataset buffer1;

  void check() {
    Variable var0{std::make_unique<Model>(indices, Dim::X, buffer0)};
    Variable var1{std::make_unique<Model>(indices, Dim::X, buffer1)};
    const auto result = buckets::concatenate(var0, var1);
    EXPECT_EQ(result.values<bucket<Dataset>>()[0],
              concatenate(buffer0.slice({Dim::X, 0, 2}),
                          buffer1.slice({Dim::X, 0, 2}), Dim::X));
    EXPECT_EQ(result.values<bucket<Dataset>>()[1],
              concatenate(buffer0.slice({Dim::X, 2, 3}),
                          buffer1.slice({Dim::X, 2, 3}), Dim::X));
  }
  void check_fail() {
    Variable var0{std::make_unique<Model>(indices, Dim::X, buffer0)};
    Variable var1{std::make_unique<Model>(indices, Dim::X, buffer1)};
    EXPECT_ANY_THROW(buckets::concatenate(var0, var1));
  }
};

TEST_F(DatasetBucketTest, concatenate) {
  buffer0.coords().set(Dim::X, column);
  buffer1.coords().set(Dim::X, column + column);
  check();
  buffer0.setData("a", column * column);
  check_fail();
  buffer1.setData("a", column);
  check();
  buffer0.setData("b", column * column);
  check_fail();
  buffer1.setData("b", column / column);
  check();
  buffer0["a"].masks().set("mask", column);
  check_fail();
  buffer1["a"].masks().set("mask", column);
  check();
  buffer0["b"].coords().set(Dim("attr"), column);
  check_fail();
  buffer1["b"].coords().set(Dim("attr"), column);
  check();
  buffer0.coords().set(Dim("scalar"), 1.0 * units::m);
  check_fail();
  buffer1.coords().set(Dim("scalar"), 1.0 * units::m);
  check();
  buffer1.coords().set(Dim("scalar2"), 1.0 * units::m);
  check_fail();
}
