// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bins.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/mean.h"
#include "scipp/dataset/sum.h"

using namespace scipp;
using namespace scipp::dataset;

static auto make_events() {
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::Y}, Shape{2}, Values{std::pair{0, 3}, std::pair{3, 7}});
  auto weights = makeVariable<double>(
      Dims{Dim::Event}, Shape{7}, sc_units::counts, Values{1, 1, 1, 1, 1, 1, 1},
      Variances{1, 1, 1, 1, 1, 1, 1});
  auto x = makeVariable<double>(Dims{Dim::Event}, Shape{7}, sc_units::us,
                                Values{1.1, 2.2, 3.3, 1.1, 2.2, 3.3, 5.5});
  DataArray buf(weights, {{Dim::X, x}});
  return make_bins(indices, Dim::Event, buf);
}

static auto make_events_array_default_weights() {
  return DataArray(make_events(),
                   {{Dim::Y, makeVariable<double>(Dimensions{Dim::Y, 2})}});
}

static auto make_histogram() {
  auto edges = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}},
                                    sc_units::us, Values{0, 2, 4, 1, 3, 5});
  auto data = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                   Values{2.0, 3.0, 2.0, 3.0});

  return DataArray(data, {{Dim::X, edges}});
}

TEST(EventsDataOperationsConsistencyTest, multiply) {
  // Apart from uncertainties (which cannot be handled by buckets::scale, since
  // it would broadcast), the order of operations does not matter. We can either
  // first multiply and then histogram, or first histogram and then multiply.
  const auto events = make_events_array_default_weights();
  auto edges = makeVariable<double>(Dimensions{Dim::X, 4}, sc_units::us,
                                    Values{1, 2, 3, 4});
  auto data =
      makeVariable<double>(Dimensions{Dim::X, 3}, Values{2.0, 3.0, 4.0});

  auto hist = DataArray(data, {{Dim::X, edges}});
  auto scaled = copy(events);
  buckets::scale(scaled, hist);
  auto ab = histogram(scaled, edges);
  auto ba = histogram(events, edges) * hist;

  // Case 1: 1 event per bin
  EXPECT_EQ(ab, ba);

  hist = make_histogram();
  edges = hist.coords()[Dim::X];
  scaled = copy(events);
  buckets::scale(scaled, hist);
  ab = histogram(scaled, edges);
  ba = histogram(events, edges) * hist;

  // Case 2: Multiple events per bin
  EXPECT_EQ(ab, ba);
}

TEST(EventsDataOperationsConsistencyTest, concatenate_sum) {
  const auto events = make_events_array_default_weights();
  auto edges = makeVariable<double>(Dimensions{Dim::X, 3}, sc_units::us,
                                    Values{1, 3, 6});
  EXPECT_EQ(sum(histogram(events, edges), Dim::Y),
            histogram(buckets::concatenate(events, Dim::Y), edges));
}

TEST(EventsDataOperationsConsistencyTest, concatenate_multiply_sum) {
  const auto events = make_events_array_default_weights();
  auto edges = makeVariable<double>(Dimensions{Dim::X, 3}, sc_units::us,
                                    Values{1, 3, 5});
  auto data = makeVariable<double>(Dimensions{Dim::X, 2}, Values{2.0, 3.0});
  auto hist = DataArray(data, {{Dim::X, edges}});

  auto m = copy(events);
  buckets::scale(m, hist); // scale ~ multiply (m)
  auto hcm = histogram(buckets::concatenate(m, Dim::Y), edges);

  auto mc = buckets::concatenate(events, Dim::Y);
  buckets::scale(mc, hist);
  auto hmc = histogram(mc, edges);

  auto mhc = hist * histogram(buckets::concatenate(events, Dim::Y), edges);

  auto msh = hist * sum(histogram(events, edges), Dim::Y);
  auto shm = sum(histogram(m, edges), Dim::Y);
  auto smh = sum(hist * histogram(events, edges), Dim::Y);

  EXPECT_EQ(hcm, hmc);
  EXPECT_EQ(hcm, mhc);
  EXPECT_EQ(hcm, msh);
  EXPECT_EQ(hcm, shm);
  EXPECT_EQ(hcm, smh);
}
