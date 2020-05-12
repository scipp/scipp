// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <numeric>

#include <benchmark/benchmark.h>

#include "random.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"
#include "scipp/variable/operations.h"

using namespace scipp;

auto make_2d_events_coord(const scipp::index size, const scipp::index count) {
  auto var = makeVariable<event_list<double>>(Dims{Dim::X}, Shape{size});
  auto vals = var.values<event_list<double>>();
  Random rand(0.0, 1000.0);
  for (scipp::index i = 0; i < size; ++i) {
    auto data = rand(count);
    vals[i].assign(data.begin(), data.end());
  }
  return var;
}

auto make_2d_events_default_weights(const scipp::index size,
                                    const scipp::index count) {
  auto weights = makeVariable<double>(Dims{Dim::X}, Shape{size}, units::counts,
                                      Values{}, Variances{});
  return DataArray(weights, {{Dim::Y, make_2d_events_coord(size, count)}});
}

auto make_2d_events(const scipp::index size, const scipp::index count) {
  auto coord = make_2d_events_coord(size, count);
  auto data =
      makeVariable<double>(Dims{}, Shape{}, Values{0.0}, Variances{0.0}) *
      coord;

  return DataArray(std::move(data), {{Dim::Y, std::move(coord)}});
}

static void BM_histogram(benchmark::State &state) {
  const scipp::index nEvent = state.range(0);
  const scipp::index nEdge = state.range(1);
  const scipp::index nHist = 1e7 / nEvent;
  const bool linear = state.range(2);
  const bool data = state.range(3);
  const auto events = data ? make_2d_events(nHist, nEvent)
                           : make_2d_events_default_weights(nHist, nEvent);
  std::vector<double> edges_(nEdge);
  std::iota(edges_.begin(), edges_.end(), 0.0);
  if (!linear)
    edges_.back() += 0.0001;
  auto edges = makeVariable<double>(Dims{Dim::Y}, Shape{nEdge},
                                    Values(edges_.begin(), edges_.end()));
  edges *= 1000.0 / nEdge * units::one; // ensure all events are in range
  for (auto _ : state) {
    benchmark::DoNotOptimize(histogram(events, edges));
  }
  state.SetItemsProcessed(state.iterations() * nHist * nEvent);
  state.SetBytesProcessed(state.iterations() * nHist *
                          ((data ? 3 : 1) * nEvent + 2 * (nEdge - 1)) *
                          sizeof(double));
  state.counters["const-width-bins"] = linear;
  state.counters["events-with-data"] = data;
}

// Params are:
// - nEvent
// - nEdge
// - constant-width-bins
// - events with data
BENCHMARK(BM_histogram)
    ->RangeMultiplier(2)
    ->Ranges({{64, 2 << 14}, {128, 2 << 11}, {false, true}, {false, true}});

BENCHMARK_MAIN();
