// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bins.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/reduction.h"

using namespace scipp;
using namespace scipp::dataset;

static auto make_events() {
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::Y}, Shape{2}, Values{std::pair{0, 3}, std::pair{3, 7}});
  auto weights = makeVariable<double>(Dims{Dim::Event}, Shape{7}, units::counts,
                                      Values{1, 1, 1, 1, 1, 1, 1},
                                      Variances{1, 1, 1, 1, 1, 1, 1});
  auto x = makeVariable<double>(Dims{Dim::Event}, Shape{7}, units::us,
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
                                    units::us, Values{0, 2, 4, 1, 3, 5});
  auto data = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                   Values{2.0, 3.0, 2.0, 3.0},
                                   Variances{0.3, 0.4, 0.3, 0.4});

  return DataArray(data, {{Dim::X, edges}});
}

TEST(EventsDataOperationsConsistencyTest, multiply) {
  // Apart from uncertainties, the order of operations does not matter. We can
  // either first multiply and then histogram, or first histogram and then
  // multiply.
  const auto events = make_events_array_default_weights();
  auto edges = makeVariable<double>(Dimensions{Dim::X, 4}, units::us,
                                    Values{1, 2, 3, 4});
  auto data = makeVariable<double>(Dimensions{Dim::X, 3}, Values{2.0, 3.0, 4.0},
                                   Variances{0.3, 0.4, 0.5});

  auto hist = DataArray(data, {{Dim::X, edges}});
  auto scaled = copy(events);
  buckets::scale(scaled, hist);
  auto ab = histogram(scaled, edges);
  auto ba = histogram(events, edges) * hist;

  // Case 1: 1 event per bin => uncertainties are the same
  EXPECT_EQ(ab, ba);

  hist = make_histogram();
  edges = Variable(hist.coords()[Dim::X]);
  scaled = copy(events);
  buckets::scale(scaled, hist);
  ab = histogram(scaled, edges);
  ba = histogram(events, edges) * hist;

  // Case 2: Multiple events per bin => uncertainties differ, remove before
  // comparison.
  ab.data().setVariances(Variable());
  ba.data().setVariances(Variable());
  EXPECT_EQ(ab, ba);
}

TEST(EventsDataOperationsConsistencyTest, concatenate_sum) {
  const auto events = make_events_array_default_weights();
  auto edges =
      makeVariable<double>(Dimensions{Dim::X, 3}, units::us, Values{1, 3, 6});
  EXPECT_EQ(sum(histogram(events, edges), Dim::Y),
            histogram(buckets::concatenate(events, Dim::Y), edges));
}

TEST(EventsDataOperationsConsistencyTest, concatenate_multiply_sum) {
  const auto events = make_events_array_default_weights();
  auto edges =
      makeVariable<double>(Dimensions{Dim::X, 3}, units::us, Values{1, 3, 5});
  auto data = makeVariable<double>(Dimensions{Dim::X, 2}, Values{2.0, 3.0},
                                   Variances{0.3, 0.4});
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

  // Same variances among "histogram after multiply" group
  EXPECT_EQ(hcm, hmc);
  EXPECT_EQ(hcm, shm);

  // Same variances among "multiply after histogram" group
  EXPECT_EQ(mhc, msh);
  // ... except that summing last also leads to smaller variances
  EXPECT_NE(mhc, smh);

  // Cross-group: Uncertainties differ due to multiple events per bin, remove.
  hcm.data().setVariances(Variable());
  mhc.data().setVariances(Variable());
  msh.data().setVariances(Variable());
  smh.data().setVariances(Variable());
  EXPECT_EQ(hcm, mhc);
  EXPECT_EQ(hcm, msh);
  EXPECT_EQ(hcm, smh);
}
