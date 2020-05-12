// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/unaligned.h"

using namespace scipp;
using namespace scipp::dataset;

struct HistogramHelpersTest : public ::testing::Test {
protected:
  Variable dataX = makeVariable<double>(Dims{Dim::X}, Shape{2});
  Variable dataY = makeVariable<double>(Dims{Dim::Y}, Shape{2});
  Variable dataXY = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3});
  Variable edgesX = makeVariable<double>(Dims{Dim::X}, Shape{3});
  Variable edgesY = makeVariable<double>(Dims{Dim::Y}, Shape{4});
  Variable coordX = makeVariable<double>(Dims{Dim::X}, Shape{2});
  Variable coordY = makeVariable<double>(Dims{Dim::Y}, Shape{3});
};

TEST_F(HistogramHelpersTest, edge_dimension) {
  const auto histX = DataArray(dataX, {{Dim::X, edgesX}});
  EXPECT_EQ(edge_dimension(histX), Dim::X);

  const auto histX2d = DataArray(dataXY, {{Dim::X, edgesX}});
  EXPECT_EQ(edge_dimension(histX2d), Dim::X);

  const auto histY2d = DataArray(dataXY, {{Dim::X, coordX}, {Dim::Y, edgesY}});
  EXPECT_EQ(edge_dimension(histY2d), Dim::Y);

  const auto hist2d = DataArray(dataXY, {{Dim::X, edgesX}, {Dim::Y, edgesY}});
  EXPECT_THROW(edge_dimension(hist2d), except::BinEdgeError);

  EXPECT_THROW(edge_dimension(DataArray(dataX, {{Dim::X, coordX}})),
               except::BinEdgeError);
  EXPECT_THROW(edge_dimension(DataArray(dataX, {{Dim::X, coordY}})),
               except::BinEdgeError);
  EXPECT_THROW(edge_dimension(DataArray(dataX, {{Dim::Y, coordX}})),
               except::BinEdgeError);
  EXPECT_THROW(edge_dimension(DataArray(dataX, {{Dim::Y, coordY}})),
               except::BinEdgeError);

  // Coord length X is 2 and data does not depend on X, but this is *not*
  // interpreted as a single-bin histogram.
  EXPECT_THROW(edge_dimension(DataArray(dataY, {{Dim::X, coordX}})),
               except::BinEdgeError);
}

TEST_F(HistogramHelpersTest, is_histogram) {
  const auto histX = DataArray(dataX, {{Dim::X, edgesX}});
  EXPECT_TRUE(is_histogram(histX, Dim::X));
  EXPECT_FALSE(is_histogram(histX, Dim::Y));

  const auto histX2d = DataArray(dataXY, {{Dim::X, edgesX}});
  EXPECT_TRUE(is_histogram(histX2d, Dim::X));
  EXPECT_FALSE(is_histogram(histX2d, Dim::Y));

  const auto histY2d = DataArray(dataXY, {{Dim::X, coordX}, {Dim::Y, edgesY}});
  EXPECT_FALSE(is_histogram(histY2d, Dim::X));
  EXPECT_TRUE(is_histogram(histY2d, Dim::Y));

  EXPECT_FALSE(is_histogram(DataArray(dataX, {{Dim::X, coordX}}), Dim::X));
  EXPECT_FALSE(is_histogram(DataArray(dataX, {{Dim::X, coordY}}), Dim::X));
  EXPECT_FALSE(is_histogram(DataArray(dataX, {{Dim::Y, coordX}}), Dim::X));
  EXPECT_FALSE(is_histogram(DataArray(dataX, {{Dim::Y, coordY}}), Dim::X));

  // Coord length X is 2 and data does not depend on X, but this is *not*
  // interpreted as a single-bin histogram.
  EXPECT_FALSE(is_histogram(DataArray(dataY, {{Dim::X, coordX}}), Dim::X));

  const auto events = makeVariable<event_list<double>>(Dims{}, Shape{});
  EXPECT_FALSE(is_histogram(DataArray(events, {{Dim::X, coordX}}), Dim::X));
}

DataArray make_1d_events_default_weights() {
  DataArray events(makeVariable<double>(Dims{Dim::X}, Shape{3}, units::counts,
                                        Values{1, 1, 1}, Variances{1, 1, 1}));
  auto var = makeVariable<event_list<double>>(Dims{Dim::X}, Shape{3});
  var.values<event_list<double>>()[0] = {1.5, 2.5, 3.5, 4.5, 5.5};
  var.values<event_list<double>>()[1] = {3.5, 4.5, 5.5, 6.5, 7.5};
  var.values<event_list<double>>()[2] = {-1, 0, 0, 1, 1, 2, 2, 2, 4, 4, 4, 6};

  events.coords().set(Dim::Y, var);
  return events;
}

TEST(HistogramTest, fail_edges_not_sorted) {
  auto events = make_1d_events_default_weights();
  ASSERT_THROW(dataset::histogram(
                   events, makeVariable<double>(Dims{Dim::Y}, Shape{6},
                                                Values{1, 3, 2, 4, 5, 6})),
               except::BinEdgeError);
}

auto make_single_events() {
  Dataset events;
  auto x = makeVariable<event_list<double>>(Dims{}, Shape{});
  x.values<event_list<double>>()[0] = {0, 1, 1, 2, 3};
  events.coords().set(Dim::X, x);
  events.setData("events", makeVariable<double>(Dims{}, Shape{}, units::counts,
                                                Values{1}, Variances{1}));
  return events;
}

DataArray make_expected(const Variable &var, const Variable &edges) {
  auto dim = var.dims().inner();
  std::map<Dim, Variable> coords = {{dim, edges}};
  auto expected = DataArray(var, coords, {}, {}, "events");
  return expected;
}

TEST(HistogramTest, below) {
  const auto events = make_single_events();
  auto edges =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{-2.0, -1.0, 0.0});
  auto hist = dataset::histogram(events["events"], edges);
  std::map<Dim, Variable> coords = {{Dim::X, edges}};
  auto expected =
      make_expected(makeVariable<double>(Dims{Dim::X}, Shape{2}, units::counts,
                                         Values{0, 0}, Variances{0, 0}),
                    edges);
  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, between) {
  const auto events = make_single_events();
  auto edges =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1.5, 1.6, 1.7});
  auto hist = dataset::histogram(events["events"], edges);
  auto expected =
      make_expected(makeVariable<double>(Dims{Dim::X}, Shape{2}, units::counts,
                                         Values{0, 0}, Variances{0, 0}),
                    edges);
  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, above) {
  const auto events = make_single_events();
  auto edges =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{3.5, 4.5, 5.5});
  auto hist = dataset::histogram(events["events"], edges);
  auto expected =
      make_expected(makeVariable<double>(Dims{Dim::X}, Shape{2}, units::counts,
                                         Values{0, 0}, Variances{0, 0}),
                    edges);
  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, data_view) {
  auto events = make_1d_events_default_weights();
  std::vector<double> ref{1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 2, 3, 0, 3, 0};
  auto edges =
      makeVariable<double>(Dims{Dim::Y}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  auto hist = dataset::histogram(events, edges);
  auto expected = make_expected(
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 5}, units::counts,
                           Values(ref.begin(), ref.end()),
                           Variances(ref.begin(), ref.end())),
      edges);

  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, dense) {
  auto events = make_1d_events_default_weights();
  auto edges1 =
      makeVariable<double>(Dims{Dim::Y}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  auto edges2 = makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1, 3, 6});
  auto expected = dataset::histogram(events, edges2);
  auto dense = dataset::histogram(events, edges1);
  EXPECT_THROW(dataset::histogram(dense, edges2), except::BinEdgeError);
  dense.coords().erase(Dim::Y);
  dense.coords().set(Dim::Y,
                     makeVariable<double>(Dims{Dim::Y}, Shape{5},
                                          Values{1.5, 2.5, 3.5, 4.5, 5.5}));
  EXPECT_EQ(dataset::histogram(dense, edges2), expected);
}

TEST(HistogramTest, drops_other_event_coords) {
  auto events = make_1d_events_default_weights();
  events.coords().set(Dim("pulse-time"), events.coords()[Dim::Y]);
  std::vector<double> ref{1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 2, 3, 0, 3, 0};
  auto edges =
      makeVariable<double>(Dims{Dim::Y}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  auto hist = dataset::histogram(events, edges);
  auto expected = make_expected(
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 5}, units::counts,
                           Values(ref.begin(), ref.end()),
                           Variances(ref.begin(), ref.end())),
      edges);

  EXPECT_FALSE(hist.coords().contains(Dim("pulse=time")));
  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, keeps_scalar_coords) {
  auto events = make_1d_events_default_weights();
  events.coords().set(Dim("scalar"), makeVariable<double>(Values{1.2}));
  auto edges = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 6});
  auto hist = dataset::histogram(events, edges);
  EXPECT_TRUE(hist.coords().contains(Dim("scalar")));
}

TEST(HistogramTest, weight_lists) {
  Variable data = makeVariable<event_list<double>>(Dimensions{{Dim::X, 3}},
                                                   Values{}, Variances{});
  data.values<event_list<double>>()[0] = {1, 1, 1, 2, 2};
  data.values<event_list<double>>()[1] = {2, 2, 2, 2, 2};
  data.values<event_list<double>>()[2] = {1, 1, 1, 1, 1, 1, 1,
                                          1, 1, 1, 1, 1, 1};
  data.variances<event_list<double>>()[0] = {1, 1, 1, 2, 2};
  data.variances<event_list<double>>()[1] = {2, 2, 2, 2, 2};
  data.variances<event_list<double>>()[2] = {1, 1, 1, 1, 1, 1, 1,
                                             1, 1, 1, 1, 1, 1};
  data.setUnit(units::counts);
  auto events = make_1d_events_default_weights();
  events.setData(data);
  auto edges =
      makeVariable<double>(Dims{Dim::Y}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  std::vector<double> ref{1, 1, 1, 2, 2, 0, 0, 2, 2, 2, 2, 3, 0, 3, 0};
  auto expected = make_expected(
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 5}, units::counts,
                           Values(ref.begin(), ref.end()),
                           Variances(ref.begin(), ref.end())),
      edges);

  EXPECT_EQ(dataset::histogram(events, edges), expected);
}

TEST(HistogramTest, dataset_realigned) {
  Dataset events;
  const auto coord =
      makeVariable<double>(Dims{Dim::Y}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  events.setData("a", unaligned::realign(make_1d_events_default_weights(),
                                         {{Dim::Y, coord}}));
  auto b_events = make_1d_events_default_weights();
  b_events.coords()[Dim::Y] += makeVariable<double>(Values{1.0});
  events.setData("b", unaligned::realign(b_events, {{Dim::Y, coord}}));

  std::vector<double> a{1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 2, 3, 0, 3, 0};
  std::vector<double> b{0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 2, 2, 3, 0, 3};
  Dataset expected;
  expected.setCoord(Dim::Y, coord);
  expected.setData("a", makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 5},
                                             units::counts,
                                             Values(a.begin(), a.end()),
                                             Variances(a.begin(), a.end())));
  expected.setData("b", makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 5},
                                             units::counts,
                                             Values(b.begin(), b.end()),
                                             Variances(b.begin(), b.end())));

  EXPECT_EQ(dataset::histogram(events), expected);
}

TEST(HistogramTest, dataset_realigned2) {
  // Similar to `dataset_realigned`, but testing vs direct histogram of items
  Dataset events;
  auto a = make_1d_events_default_weights();
  auto b = make_1d_events_default_weights();
  b.coords()[Dim::Y] += makeVariable<double>(Values{1.0});
  const auto bins =
      makeVariable<double>(Dims{Dim::Y}, Shape{6}, Values{1, 2, 3, 4, 5, 6});

  Dataset expected;
  expected.setData("a", histogram(a, bins));
  expected.setData("b", histogram(b, bins));

  events.setData("a", unaligned::realign(a, {{Dim::Y, bins}}));
  events.setData("b", unaligned::realign(b, {{Dim::Y, bins}}));

  EXPECT_EQ(dataset::histogram(events), expected);
}
