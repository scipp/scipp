// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <cmath>

#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/math.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/variable_factory.h"

using namespace scipp;
using namespace scipp::dataset;

class DataArrayBinsTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable data =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  Variable var = make_bins(indices, Dim::X, copy(buffer));
};

TEST_F(DataArrayBinsTest, concatenate_dim_1d_data_array_buffer) {
  Variable expected_indices =
      makeVariable<scipp::index_pair>(Values{std::pair{0, 4}});
  Variable expected = make_bins(expected_indices, Dim::X, buffer);
  EXPECT_EQ(buckets::concatenate(var, Dim::Y), expected);
}

TEST_F(DataArrayBinsTest, concatenate_dim_1d_variable_buffer) {
  Variable expected_indices =
      makeVariable<scipp::index_pair>(Values{std::pair{0, 4}});
  Variable expected = make_bins(expected_indices, Dim::X, data);
  EXPECT_EQ(buckets::concatenate(bins_view<DataArray>(var).data(), Dim::Y),
            expected);
}

TEST_F(DataArrayBinsTest, concatenate_dim_1d_masked) {
  const auto y = makeVariable<double>(dims);
  const auto scalar = makeVariable<double>(Values{1.2});
  const auto mask = makeVariable<bool>(dims, Values{true, false});
  const auto scalar_mask = makeVariable<bool>(Values{false});
  DataArray a(var, {{Dim::Y, y}, {Dim("scalar"), scalar}},
              {{"mask", mask}, {"scalar", scalar_mask}});
  auto expected = copy(a.slice({Dim::Y, 1}));
  expected.coords().erase(Dim::Y);
  expected.masks().erase("mask");
  EXPECT_EQ(buckets::concatenate(a, Dim::Y), expected);
}

TEST(DataArrayBins2dTest, concatenate_dim_2d) {
  Variable indicesZY =
      makeVariable<scipp::index_pair>(Dims{Dim::Z, Dim::Y}, Shape{2, 2},
                                      Values{std::pair{0, 2}, std::pair{2, 3},
                                             std::pair{4, 6}, std::pair{6, 6}});
  Variable data =
      makeVariable<double>(Dims{Dim::X}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  Variable zy = make_bins(indicesZY, Dim::X, buffer);

  // Note that equality ignores data not in any bin.
  Variable indicesZ = makeVariable<scipp::index_pair>(
      Dims{Dim::Z}, Shape{2}, Values{std::pair{0, 3}, std::pair{4, 6}});
  Variable z = make_bins(indicesZ, Dim::X, buffer);

  Variable indicesY = makeVariable<scipp::index_pair>(
      Dims{Dim::Y}, Shape{2}, Values{std::pair{0, 4}, std::pair{4, 5}});
  data = makeVariable<double>(Dims{Dim::X}, Shape{5}, Values{1, 2, 5, 6, 3});
  buffer = DataArray(data, {{Dim::X, data + data}});
  Variable y = make_bins(indicesY, Dim::X, buffer);

  EXPECT_EQ(buckets::concatenate(zy, Dim::Y), z);
  EXPECT_EQ(buckets::concatenate(zy, Dim::Z), y);
  EXPECT_EQ(
      bins_sum(buckets::concatenate(buckets::concatenate(zy, Dim::Y), Dim::Z)),
      bins_sum(buckets::concatenate(buckets::concatenate(zy, Dim::Z), Dim::Y)));
}

TEST_F(DataArrayBinsTest, concatenate) {
  const auto result = buckets::concatenate(var, var * (3.0 * sc_units::one));
  Variable out_indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 4}, std::pair{4, 8}});
  Variable out_data = makeVariable<double>(Dims{Dim::X}, Shape{8},
                                           Values{1, 2, 3, 6, 3, 4, 9, 12});
  Variable out_x = makeVariable<double>(Dims{Dim::X}, Shape{8},
                                        Values{2, 4, 2, 4, 6, 8, 6, 8});
  DataArray out_buffer = DataArray(out_data, {{Dim::X, out_x}});
  EXPECT_EQ(result, make_bins(out_indices, Dim::X, out_buffer));

  // "in-place" append gives same as concatenate
  buckets::append(var, var * (3.0 * sc_units::one));
  EXPECT_EQ(result, var);
  buckets::append(var, -var);
}

TEST_F(DataArrayBinsTest, concatenate_with_broadcast) {
  auto var2 = copy(var).rename_dims({{Dim::Y, Dim::Z}});
  var2 *= 3.0 * sc_units::one;
  const auto result = buckets::concatenate(var, var2);
  Variable out_indices = makeVariable<scipp::index_pair>(
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
  EXPECT_EQ(result, make_bins(out_indices, Dim::X, out_buffer));

  // Broadcast not possible for in-place append
  EXPECT_THROW(buckets::append(var, var2), except::DimensionError);
}

TEST_F(DataArrayBinsTest, histogram) {
  Variable weights =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, sc_units::counts,
                           Values{1, 2, 3, 4}, Variances{1, 2, 3, 4});
  DataArray events = DataArray(weights, {{Dim::Z, data}});
  Variable buckets = make_bins(indices, Dim::X, events);
  // `buckets` *does not* depend on the histogramming dimension
  const auto bin_edges =
      makeVariable<double>(Dims{Dim::Z}, Shape{4}, Values{0, 1, 2, 4});
  EXPECT_EQ(buckets::histogram(buckets, bin_edges),
            makeVariable<double>(Dims{Dim::Y, Dim::Z}, Shape{2, 3},
                                 sc_units::counts, Values{0, 1, 2, 0, 0, 3},
                                 Variances{0, 1, 2, 0, 0, 3}));
}

TEST_F(DataArrayBinsTest, histogram_masked) {
  Variable weights =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, sc_units::counts,
                           Values{1, 2, 3, 4}, Variances{1, 2, 3, 4});
  Variable mask = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                     Values{false, false, true, false});
  DataArray events = DataArray(weights, {{Dim::Z, data}}, {{"mask", mask}});
  Variable buckets = make_bins(indices, Dim::X, events);
  // `buckets` *does not* depend on the histogramming dimension
  const auto bin_edges =
      makeVariable<double>(Dims{Dim::Z}, Shape{4}, Values{0, 1, 2, 4});
  EXPECT_EQ(buckets::histogram(buckets, bin_edges),
            makeVariable<double>(Dims{Dim::Y, Dim::Z}, Shape{2, 3},
                                 sc_units::counts, Values{0, 1, 2, 0, 0, 0},
                                 Variances{0, 1, 2, 0, 0, 0}));
}

TEST_F(DataArrayBinsTest, histogram_existing_dim) {
  Variable weights =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, sc_units::counts,
                           Values{1, 2, 3, 4}, Variances{1, 2, 3, 4});
  DataArray events = DataArray(weights, {{Dim::Y, data}});
  Variable buckets = make_bins(indices, Dim::X, events);
  // `buckets` *does* depend on the histogramming dimension
  const auto bin_edges =
      makeVariable<double>(Dims{Dim::Y}, Shape{4}, Values{0, 1, 2, 4});
  const auto expected =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, sc_units::counts,
                           Values{0, 1, 5}, Variances{0, 1, 5});
  EXPECT_EQ(buckets::histogram(buckets, bin_edges), expected);

  // Histogram data array containing binned variable
  DataArray a(buckets);
  EXPECT_EQ(histogram(a, bin_edges),
            DataArray(expected, {{Dim::Y, bin_edges}}));
  // Masked data array
  a.masks().set(
      "mask", makeVariable<bool>(Dims{Dim::Y}, Shape{2}, Values{false, true}));
  EXPECT_EQ(
      histogram(a, bin_edges),
      DataArray(makeVariable<double>(Dims{Dim::Y}, Shape{3}, sc_units::counts,
                                     Values{0, 1, 2}, Variances{0, 1, 2}),
                {{Dim::Y, bin_edges}}));
}

TEST_F(DataArrayBinsTest, operations_on_empty) {
  const Variable empty_indices = makeVariable<scipp::index_pair>(
      Dimensions{{Dim::Y, 0}, {Dim::Z, 0}}, Values{});
  const Variable binned = make_bins(empty_indices, Dim::X, data);

  EXPECT_EQ(abs(binned), binned);
  EXPECT_EQ(binned, binned * binned);
  EXPECT_EQ(binned, binned * (2 * sc_units::one));
}

class DataArrayBinsMapTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable data =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  Variable weights = makeVariable<double>(
      Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4}, Variances{1, 2, 3, 4});
  DataArray events = DataArray(weights, {{Dim::Z, data}});
  Variable buckets = make_bins(indices, Dim::X, events);
  // `buckets` *does not* depend on the histogramming dimension
  Variable bin_edges =
      makeVariable<double>(Dims{Dim::Z}, Shape{4}, Values{0, 1, 2, 4});
  DataArray histogram =
      DataArray(makeVariable<double>(Dims{Dim::Z}, Shape{3}, sc_units::K,
                                     Values{1, 2, 4}),
                {{Dim::Z, bin_edges}});
};

TEST_F(DataArrayBinsMapTest, map) {
  const auto &coord = bins_view<DataArray>(buckets).coords()[Dim::Z];
  auto fill_value = makeVariable<double>(sc_units::K, Values{1234});
  const auto out = buckets::map(histogram, coord, Dim::Z, fill_value);
  // event coords 1,2,3,4
  // histogram:
  // | 1 | 2 | 4 |
  // 0   1   2   4
  const auto expected_scale = makeVariable<double>(
      Dims{Dim::X}, Shape{4}, sc_units::K, Values{2, 4, 4, 1234});
  EXPECT_EQ(out, make_bins(indices, Dim::X, expected_scale));

  // Mapping result can be used to scale
  auto scaled = buckets * out;
  const auto expected = make_bins(indices, Dim::X, events * expected_scale);
  EXPECT_EQ(scaled, expected);

  // Mapping and scaling also works for slices
  histogram.setUnit(sc_units::one); // cannot change unit of slice
  fill_value.setUnit(sc_units::one);
  auto partial = buckets;
  for (auto s : {Slice(Dim::Y, 0), Slice(Dim::Y, 1)})
    partial.slice(s) *=
        buckets::map(histogram, coord.slice(s), Dim::Z, fill_value);
  variable::variableFactory().set_elem_unit(partial, sc_units::K);
  EXPECT_EQ(partial, expected);
}

TEST_F(DataArrayBinsMapTest, fail_no_bin_edges) {
  const auto &coord = bins_view<DataArray>(buckets).coords()[Dim::Z];
  histogram.coords().set(Dim::Z, bin_edges.slice({Dim::Z, 1, 4}));
  EXPECT_THROW_DISCARD(buckets::map(histogram, coord, Dim::Z),
                       except::BinEdgeError);
}

TEST_F(DataArrayBinsMapTest, map_masked_values_replaced_by_fill_value) {
  const auto &coord = bins_view<DataArray>(buckets).coords()[Dim::Z];
  histogram.masks().set(
      "mask", makeVariable<bool>(histogram.dims(), Values{false, true, false}));
  const auto fill_value = makeVariable<double>(sc_units::K, Values{1234});
  const auto out = buckets::map(histogram, coord, Dim::Z, fill_value);
  const auto expected_scale = makeVariable<double>(
      Dims{Dim::X}, Shape{4}, sc_units::K, Values{1234, 4, 4, 1234});
  EXPECT_EQ(out, make_bins(indices, Dim::X, expected_scale));
}

class DataArrayBinsScaleTest : public ::testing::Test {
protected:
  auto make_indices() const {
    return makeVariable<scipp::index_pair>(
        Dims{Dim::Y, Dim::X}, Shape{2, 1},
        Values{std::pair{0, 3}, std::pair{3, 7}});
  }
  auto make_events() const {
    auto weights = makeVariable<double>(
        Dims{Dim("event")}, Shape{7}, sc_units::us, Values{1, 2, 1, 3, 1, 1, 1},
        Variances{1, 3, 1, 2, 1, 1, 1});
    auto coord =
        makeVariable<double>(Dims{Dim("event")}, Shape{7}, sc_units::us,
                             Values{1.1, 2.2, 3.3, 1.1, 2.2, 3.3, 5.5});
    return DataArray(weights, {{Dim::X, coord}});
  }

  auto make_histogram() const {
    auto edges = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                      sc_units::us, Values{0, 2, 4, 1, 3, 5});
    auto data = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                     Values{2.0, 3.0, 2.0, 3.0},
                                     Variances{0.3, 0.4, 0.3, 0.4});
    return DataArray(data, {{Dim::X, edges}});
  }

  auto make_histogram_no_variance() const {
    auto edges = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                      sc_units::us, Values{0, 2, 4, 1, 3, 5});
    auto data = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                     Values{2.0, 3.0, 2.0, 3.0});
    return DataArray(data, {{Dim::X, edges}});
  }

  auto make_buckets(const DataArray &events,
                    const std::map<Dim, Variable> &coords = {}) const {
    auto array = DataArray(make_bins(make_indices(), Dim("event"), events));
    for (const auto &[dim, coord] : coords)
      array.coords().set(dim, coord);
    return array;
  }
};

TEST_F(DataArrayBinsScaleTest, fail_events_op_non_histogram) {
  const auto events = make_events();
  auto coord = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                    sc_units::us, Values{0, 2, 1, 3});
  auto data = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                   Values{2.0, 3.0, 2.0, 3.0},
                                   Variances{0.3, 0.4, 0.3, 0.4});
  DataArray not_hist(data, {{Dim::X, coord}});

  // Fail due to coord mismatch between event coord and dense coord
  EXPECT_THROW_DISCARD(events * not_hist, except::CoordMismatchError);
  EXPECT_THROW_DISCARD(not_hist * events, except::CoordMismatchError);
  EXPECT_THROW_DISCARD(events / not_hist, except::CoordMismatchError);

  auto buckets = make_buckets(events);

  // Fail because non-event operand has to be a histogram
  EXPECT_THROW(buckets::scale(buckets, not_hist), except::BinEdgeError);
  // We have a single bin in X, so setting the "same" coord as in `not_hist`
  // we have a matching coord, it it would be a bin-edge coord on `buckets`.
  buckets.coords().set(Dim::X, not_hist.coords()[Dim::X]);
  EXPECT_THROW(buckets::scale(buckets, not_hist), except::BinEdgeError);
}

TEST_F(DataArrayBinsScaleTest, events_times_histogram) {
  const auto events = make_events();
  const auto hist = make_histogram();
  auto buckets = make_buckets(events);
  ASSERT_THROW(buckets::scale(buckets, hist), except::VariancesError);
}

TEST_F(DataArrayBinsScaleTest, events_times_histogram_without_variances) {
  const auto events = make_events();
  const auto hist = make_histogram_no_variance();
  auto buckets = make_buckets(events);
  buckets::scale(buckets, hist);

  auto expected_weights = makeVariable<double>(
      Dims{Dim("event")}, Shape{7}, sc_units::us, Values{1, 2, 1, 3, 1, 1, 1},
      Variances{1, 3, 1, 2, 1, 1, 1});
  // Last event is out of bounds and scaled to 0.0
  expected_weights *= makeVariable<double>(
      Dims{Dim("event")}, Shape{7}, Values{2.0, 3.0, 3.0, 2.0, 2.0, 3.0, 0.0});
  auto expected_events = make_events();
  copy(expected_weights, expected_events.data());

  EXPECT_EQ(buckets, make_buckets(expected_events));
}

TEST_F(DataArrayBinsScaleTest, events_times_histogram_drops_applied_mask) {
  const auto events = make_events();
  auto hist = make_histogram_no_variance();
  hist.masks().set(
      "mask", makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false}));
  auto buckets = make_buckets(events);
  buckets::scale(buckets, hist);

  auto expected_weights = makeVariable<double>(
      Dims{Dim("event")}, Shape{7}, sc_units::us, Values{1, 2, 1, 3, 1, 1, 1},
      Variances{1, 3, 1, 2, 1, 1, 1});
  // Masked events are zeroed.
  // Last event is out of bounds and scaled to 0.0
  expected_weights *= makeVariable<double>(
      Dims{Dim("event")}, Shape{7}, Values{0.0, 3.0, 3.0, 0.0, 0.0, 3.0, 0.0});
  auto expected_events = make_events();
  copy(expected_weights, expected_events.data());

  EXPECT_TRUE(buckets.masks().empty());
  EXPECT_EQ(buckets, make_buckets(expected_events));
}

TEST_F(DataArrayBinsScaleTest, events_times_histogram_keeps_non_edge_mask) {
  const auto events = make_events();
  auto hist = make_histogram_no_variance();
  hist.masks().set(
      "mask", makeVariable<bool>(Dims{Dim::Y}, Shape{2}, Values{true, false}));
  auto buckets = make_buckets(events);
  buckets::scale(buckets, hist);

  auto expected_weights = makeVariable<double>(
      Dims{Dim("event")}, Shape{7}, sc_units::us, Values{1, 2, 1, 3, 1, 1, 1},
      Variances{1, 3, 1, 2, 1, 1, 1});
  // No mask has been applied.
  // Last event is out of bounds and scaled to 0.0
  expected_weights *= makeVariable<double>(
      Dims{Dim("event")}, Shape{7}, Values{2.0, 3.0, 3.0, 2.0, 2.0, 3.0, 0.0});
  auto expected_events = make_events();
  copy(expected_weights, expected_events.data());
  auto expected_buckets = make_buckets(expected_events);
  expected_buckets.masks().set("mask", hist.masks()["mask"]);

  EXPECT_EQ(buckets, expected_buckets);
}

TEST_F(DataArrayBinsScaleTest,
       events_times_histogram_fail_too_many_bucketed_dims) {
  auto x = make_histogram_no_variance();
  auto z = x.rename_dims({{Dim::X, Dim::Z}});
  z.coords().set(Dim::Z, z.coords().extract(Dim::X));
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

class DataArrayBinsPlusMinusTest : public ::testing::Test {
protected:
  auto make_events() const {
    auto weights = makeVariable<double>(
        Dims{Dim("event")}, Shape{7}, sc_units::counts,
        Values{1, 2, 1, 3, 1, 1, 1}, Variances{1, 3, 1, 2, 1, 1, 1});
    auto coord =
        makeVariable<double>(Dims{Dim("event")}, Shape{7}, sc_units::us,
                             Values{1.1, 2.2, 3.3, 1.1, 2.2, 3.3, 5.5});
    return DataArray(weights, {{Dim::X, coord}});
  }

  DataArrayBinsPlusMinusTest() {
    eventsA = make_events();
    eventsB = copy(eventsA);
    eventsB.coords()[Dim::X] += 0.01 * sc_units::us;
    eventsB = concat(std::vector{eventsB, eventsA}, Dim("event"));
    eventsB.coords()[Dim::X] += 0.02 * sc_units::us;
    a = DataArray(make_bins(makeVariable<scipp::index_pair>(
                                Dims{Dim::Y, Dim::X}, Shape{2, 1},
                                Values{std::pair{0, 3}, std::pair{3, 7}}),
                            Dim("event"), eventsA));
    b = DataArray(make_bins(makeVariable<scipp::index_pair>(
                                Dims{Dim::Y, Dim::X}, Shape{2, 1},
                                Values{std::pair{0, 5}, std::pair{5, 14}}),
                            Dim("event"), eventsB));
  }

  DataArray eventsA;
  DataArray eventsB;
  Variable edges = makeVariable<double>(Dims{Dim::X}, Shape{4}, sc_units::us,
                                        Values{0, 2, 4, 6});
  DataArray a;
  DataArray b;
};

TEST_F(DataArrayBinsPlusMinusTest, plus) {
  EXPECT_EQ(bins_sum(buckets::concatenate(a, b)), bins_sum(a) + bins_sum(b));
}

TEST_F(DataArrayBinsPlusMinusTest, minus) {
  auto tmp = -b;
  EXPECT_EQ(b.unit(), sc_units::none);
  EXPECT_EQ(tmp.unit(), sc_units::none);
  EXPECT_EQ(bins_sum(buckets::concatenate(a, -b)), bins_sum(a) - bins_sum(b));
}

TEST_F(DataArrayBinsPlusMinusTest, plus_equals) {
  auto out = copy(a);
  buckets::append(out, b);
  EXPECT_EQ(out, buckets::concatenate(a, b));
  buckets::append(out, -b);
  EXPECT_NE(out, a); // events not removed by "undo" of addition
  EXPECT_NE(bins_sum(out), bins_sum(a)); // mismatching variances
  EXPECT_EQ(out, buckets::concatenate(buckets::concatenate(a, b), -b));
}

TEST_F(DataArrayBinsPlusMinusTest, plus_equals_self) {
  auto out = copy(a);
  buckets::append(out, out);
  EXPECT_EQ(out, buckets::concatenate(a, a));
}

TEST_F(DataArrayBinsPlusMinusTest, minus_equals) {
  auto out = copy(a);
  buckets::append(out, -b);
  EXPECT_EQ(out, buckets::concatenate(a, -b));
}

class DatasetBinsTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Dimensions buffer_dims{Dim::X, 3};
  Variable indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 2}, std::pair{2, 3}});
  Variable column =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});

  void check(const Dataset &buffer0, const Dataset &buffer1) {
    Variable var0 = make_bins(indices, Dim::X, buffer0);
    Variable var1 = make_bins(indices, Dim::X, buffer1);
    const auto result = buckets::concatenate(var0, var1);
    EXPECT_EQ(result.values<core::bin<Dataset>>()[0],
              concat(std::vector{buffer0.slice({Dim::X, 0, 2}),
                                 buffer1.slice({Dim::X, 0, 2})},
                     Dim::X));
    EXPECT_EQ(result.values<core::bin<Dataset>>()[1],
              concat(std::vector{buffer0.slice({Dim::X, 2, 3}),
                                 buffer1.slice({Dim::X, 2, 3})},
                     Dim::X));
  }
  void check_fail(const Dataset &buffer0, const Dataset &buffer1) {
    Variable var0 = make_bins(indices, Dim::X, buffer0);
    Variable var1 = make_bins(indices, Dim::X, buffer1);
    EXPECT_ANY_THROW([[maybe_unused]] auto joined =
                         buckets::concatenate(var0, var1));
  }
};

TEST_F(DatasetBinsTest, concatenate) {
  Dataset buffer0({}, {{Dim::X, column}});
  Dataset buffer1({}, {{Dim::X, column + column}});
  check(buffer0, buffer1);
  buffer0.setData("a", column * column);
  check_fail(buffer0, buffer1);
  buffer1.setData("a", column);
  check(buffer0, buffer1);
  buffer0.setData("b", column * column);
  check_fail(buffer0, buffer1);
  // cppcheck-suppress duplicateExpression  # Intentional, see above.
  buffer1.setData("b", column / column);
  check(buffer0, buffer1);
  buffer0["a"].masks().set("mask", column);
  check_fail(buffer0, buffer1);
  buffer1["a"].masks().set("mask", column);
  check(buffer0, buffer1);
  buffer0.coords().set(Dim("scalar"), 1.0 * sc_units::m);
  check_fail(buffer0, buffer1);
  buffer1.coords().set(Dim("scalar"), 1.0 * sc_units::m);
  check(buffer0, buffer1);
  buffer1.coords().set(Dim("scalar2"), 1.0 * sc_units::m);
  check_fail(buffer0, buffer1);
}
