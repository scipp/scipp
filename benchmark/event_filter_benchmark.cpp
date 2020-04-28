// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include "random.h"

#include "scipp/variable/operations.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/event.h"

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

static void BM_event_filter(benchmark::State &state) {
  const scipp::index nEvent = state.range(0);
  const scipp::index nHist = 1e7 / nEvent;
  const double fraction = state.range(1) * 0.01;
  const bool data = state.range(2);
  const auto events = data ? make_2d_events(nHist, nEvent)
                           : make_2d_events_default_weights(nHist, nEvent);
  const auto interval = makeVariable<double>(Dims{Dim::Y}, Shape{2},
                                             Values{0.0, 1000 * fraction});
  for (auto _ : state) {
    benchmark::DoNotOptimize(dataset::event::filter(events, Dim::Y, interval));
  }
  state.SetItemsProcessed(state.iterations() * nHist * nEvent);
  state.SetBytesProcessed(state.iterations() * nHist * (data ? 3 : 1) * nEvent *
                          sizeof(double));
  state.counters["included-fraction"] = fraction;
  state.counters["events-with-data"] = data;
}

// Params are:
// - nEvent
// - included percent
// - events with data
BENCHMARK(BM_event_filter)
    ->RangeMultiplier(2)
    ->Ranges({{64, 2 << 14}, {10, 100}, {false, true}});

BENCHMARK_MAIN();
