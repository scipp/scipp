// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"
#include "scipp/core/histogram.h"

using namespace scipp;
using namespace scipp::core;

TEST(HistogramTest, is_histogram) {
  const auto dataX = makeVariable<double>(Dims{Dim::X}, Shape{2});
  const auto dataY = makeVariable<double>(Dims{Dim::Y}, Shape{2});
  const auto dataXY = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3});
  const auto edgesX = makeVariable<double>(Dims{Dim::X}, Shape{3});
  const auto edgesY = makeVariable<double>(Dims{Dim::Y}, Shape{4});
  const auto coordX = makeVariable<double>(Dims{Dim::X}, Shape{2});
  const auto coordY = makeVariable<double>(Dims{Dim::Y}, Shape{3});

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

  const auto sparse = makeVariable<event_list<double>>(Dims{}, Shape{});
  EXPECT_FALSE(is_histogram(DataArray(sparse, {{Dim::X, coordX}}), Dim::X));
}

DataArray make_1d_events_default_weights() {
  DataArray events(makeVariable<double>(Dims{Dim::X}, Shape{3},
                                        units::Unit(units::counts),
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
  ASSERT_THROW(
      core::histogram(events, makeVariable<double>(Dims{Dim::Y}, Shape{6},
                                                   Values{1, 3, 2, 4, 5, 6})),
      except::BinEdgeError);
}

auto make_single_sparse() {
  Dataset sparse;
  auto x = makeVariable<event_list<double>>(Dims{}, Shape{});
  x.values<event_list<double>>()[0] = {0, 1, 1, 2, 3};
  sparse.coords().set(Dim::X, x);
  sparse.setData("sparse", makeVariable<double>(Dims{}, Shape{},
                                                units::Unit(units::counts),
                                                Values{1}, Variances{1}));
  return sparse;
}

DataArray make_expected(const Variable &var, const Variable &edges) {
  auto dim = var.dims().inner();
  std::map<Dim, Variable> coords = {{dim, edges}};
  auto expected = DataArray(var, coords, {}, {}, "sparse");
  return expected;
}

TEST(HistogramTest, below) {
  const auto sparse = make_single_sparse();
  auto edges =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{-2.0, -1.0, 0.0});
  auto hist = core::histogram(sparse["sparse"], edges);
  std::map<Dim, Variable> coords = {{Dim::X, edges}};
  auto expected = make_expected(
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::Unit(units::counts),
                           Values{0, 0}, Variances{0, 0}),
      edges);
  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, between) {
  const auto sparse = make_single_sparse();
  auto edges =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1.5, 1.6, 1.7});
  auto hist = core::histogram(sparse["sparse"], edges);
  auto expected = make_expected(
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::Unit(units::counts),
                           Values{0, 0}, Variances{0, 0}),
      edges);
  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, above) {
  const auto sparse = make_single_sparse();
  auto edges =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{3.5, 4.5, 5.5});
  auto hist = core::histogram(sparse["sparse"], edges);
  auto expected = make_expected(
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::Unit(units::counts),
                           Values{0, 0}, Variances{0, 0}),
      edges);
  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, data_view) {
  auto events = make_1d_events_default_weights();
  std::vector<double> ref{1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 2, 3, 0, 3, 0};
  auto edges =
      makeVariable<double>(Dims{Dim::Y}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  auto hist = core::histogram(events, edges);
  auto expected =
      make_expected(makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 5},
                                         units::Unit(units::counts),
                                         Values(ref.begin(), ref.end()),
                                         Variances(ref.begin(), ref.end())),
                    edges);

  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, drops_other_event_coords) {
  auto events = make_1d_events_default_weights();
  events.coords().set(Dim("pulse-time"), events.coords()[Dim::Y]);
  std::vector<double> ref{1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 2, 3, 0, 3, 0};
  auto edges =
      makeVariable<double>(Dims{Dim::Y}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  auto hist = core::histogram(events, edges);
  auto expected =
      make_expected(makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 5},
                                         units::Unit(units::counts),
                                         Values(ref.begin(), ref.end()),
                                         Variances(ref.begin(), ref.end())),
                    edges);

  EXPECT_FALSE(hist.coords().contains(Dim("pulse=time")));
  EXPECT_EQ(hist, expected);
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
  auto expected =
      make_expected(makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 5},
                                         units::Unit(units::counts),
                                         Values(ref.begin(), ref.end()),
                                         Variances(ref.begin(), ref.end())),
                    edges);

  EXPECT_EQ(core::histogram(events, edges), expected);
}

// Disabled because this would require events as unaligned dataset entries.
TEST(HistogramTest, DISABLED_dataset) {
  Dataset events;
  events.setData("a", make_1d_events_default_weights());
  events.setData("b", events["a"]);

  // TODO This needs to be refactored, need to change the *unaligned* component
  // of the coord.
  events["b"].coords()[Dim::Y] += makeVariable<double>(Values{1.0});
  std::vector<double> a{1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 2, 3, 0, 3, 0};
  std::vector<double> b{0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 2, 2, 3, 0, 3};
  const auto coord =
      makeVariable<double>(Dims{Dim::Y}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  Dataset expected;
  expected.setCoord(Dim::Y, coord);
  expected.setData("a", makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 5},
                                             units::Unit(units::counts),
                                             Values(a.begin(), a.end()),
                                             Variances(a.begin(), a.end())));
  expected.setData("b", makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 5},
                                             units::Unit(units::counts),
                                             Values(b.begin(), b.end()),
                                             Variances(b.begin(), b.end())));

  EXPECT_EQ(core::histogram(events, coord), expected);
}

// Disabled because this would require events as unaligned dataset entries.
TEST(HistogramTest, DISABLED_dataset_aligned_axis) {
  Dataset events;
  auto a = make_1d_events_default_weights();
  auto b = make_1d_events_default_weights();
  b.coords()[Dim::Y] += makeVariable<double>(Values{1.0});
  const auto bins =
      makeVariable<double>(Dims{Dim::Y}, Shape{6}, Values{1, 2, 3, 4, 5, 6});

  Dataset expected;
  expected.setData("a", histogram(a, bins));
  expected.setData("b", histogram(b, bins));

  // events.setUnaligned("a", a);
  // events.setUnaligned("b", b);

  EXPECT_EQ(core::histogram(events, Dim::Y), expected);
}
