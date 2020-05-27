// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/histogram.h"
#include "scipp/dataset/reduction.h"
#include "scipp/dataset/unaligned.h"

using namespace scipp;
using namespace scipp::dataset;

static auto make_events() {
  auto var = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  var.setUnit(units::us);
  auto vals = var.values<event_list<double>>();
  vals[0] = {1.1, 2.2, 3.3};
  vals[1] = {1.1, 2.2, 3.3, 5.5};
  return var;
}

static auto make_events_array_default_weights() {
  return DataArray(makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::counts,
                                        Values{1, 1}, Variances{1, 1}),
                   {{Dim::X, make_events()},
                    {Dim::Y, makeVariable<double>(Dimensions{Dim::Y, 2})}});
}

static auto make_histogram() {
  auto edges = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}},
                                    units::us, Values{0, 2, 4, 1, 3, 5});
  auto data = makeVariable<double>(Dimensions{Dim::X, 2}, Values{2.0, 3.0},
                                   Variances{0.3, 0.4});

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
  auto realigned = unaligned::realign(events, {{Dim::X, edges}});

  auto hist = DataArray(data, {{Dim::X, edges}});
  auto ab = histogram(realigned * hist);
  auto ba = histogram(realigned) * hist;

  // Case 1: 1 event per bin => uncertainties are the same
  EXPECT_EQ(ab, ba);

  hist = make_histogram();
  edges = Variable(hist.coords()[Dim::X]);
  realigned = unaligned::realign(events, {{Dim::X, edges}});
  ab = histogram(realigned * hist);
  ba = histogram(realigned) * hist;

  // Case 2: Multiple events per bin => uncertainties differ, remove before
  // comparison.
  ab.data().setVariances(Variable());
  ba.data().setVariances(Variable());
  EXPECT_EQ(ab, ba);
}

TEST(EventsDataOperationsConsistencyTest, flatten_sum) {
  const auto events = make_events_array_default_weights();
  auto edges =
      makeVariable<double>(Dimensions{Dim::X, 3}, units::us, Values{1, 3, 6});
  EXPECT_EQ(sum(histogram(events, edges), Dim::Y),
            histogram(flatten(events, Dim::Y), edges));
}

TEST(EventsDataOperationsConsistencyTest, flatten_sum_realigned) {
  const auto events = make_events_array_default_weights();
  auto edges =
      makeVariable<double>(Dimensions{Dim::X, 3}, units::us, Values{1, 3, 6});
  const auto realigned = unaligned::realign(events, {{Dim::X, edges}});

  // Three equalities that all express the same concept:
  EXPECT_EQ(histogram(sum(realigned, Dim::Y)),
            sum(histogram(realigned), Dim::Y));

  EXPECT_EQ(histogram(sum(realigned, Dim::Y)),
            histogram(flatten(events, Dim::Y), edges));

  const auto summed = sum(realigned, Dim::Y);
  const auto flattened = flatten(events, Dim::Y);
  EXPECT_EQ(summed.unaligned(), flattened);
}

TEST(EventsDataOperationsConsistencyTest, flatten_multiply_sum) {
  const auto events = make_events_array_default_weights();
  auto edges =
      makeVariable<double>(Dimensions{Dim::X, 3}, units::us, Values{1, 3, 5});
  auto data = makeVariable<double>(Dimensions{Dim::X, 2}, Values{2.0, 3.0},
                                   Variances{0.3, 0.4});
  auto realigned = unaligned::realign(events, {{Dim::X, edges}});
  auto hist = DataArray(data, {{Dim::X, edges}});

  auto hfm = histogram(flatten((hist * realigned).unaligned(), Dim::Y), edges);

  auto hmf = histogram(
      hist * unaligned::realign(flatten(events, Dim::Y), {{Dim::X, edges}}));

  auto mhf = hist * histogram(flatten(events, Dim::Y), edges);

  auto msh = hist * sum(histogram(realigned), Dim::Y);
  auto shm = sum(histogram(hist * realigned), Dim::Y);
  auto smh = sum(hist * histogram(realigned), Dim::Y);

  // Same variances among "histogram after multiply" group
  EXPECT_EQ(hfm, hmf);
  EXPECT_EQ(hfm, shm);

  // Same variances among "multiply after histogram" group
  EXPECT_EQ(mhf, msh);
  // ... except that summing last also leads to smaller variances
  EXPECT_NE(mhf, smh);

  // Cross-group: Uncertainties differ due to multiple events per bin, remove.
  hfm.data().setVariances(Variable());
  mhf.data().setVariances(Variable());
  msh.data().setVariances(Variable());
  smh.data().setVariances(Variable());
  EXPECT_EQ(hfm, mhf);
  EXPECT_EQ(hfm, msh);
  EXPECT_EQ(hfm, smh);
}
