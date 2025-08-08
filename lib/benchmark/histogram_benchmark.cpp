// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
#include <numeric>

#include <benchmark/benchmark.h>

#include "random.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"
#include "scipp/variable/operations.h"

using namespace scipp;

auto make_2d_events(const scipp::index size, const scipp::index count) {
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{size});
  scipp::index row = 0;
  for (auto &range : indices.values<std::pair<scipp::index, scipp::index>>()) {
    range = {row, row + count};
    row += count;
  }
  auto weights =
      makeVariable<double>(Dims{Dim::Event}, Shape{row}, Values{}, Variances{});
  Random rand(0.0, 1000.0);
  auto y = makeVariable<double>(Dims{Dim::Event}, Shape{row},
                                Values(rand(size * count)));
  DataArray buf(weights, {{Dim::Y, y}});
  return DataArray(make_bins(indices, Dim::Event, buf));
}

static void BM_histogram(benchmark::State &state) {
  const scipp::index nEvent = state.range(0);
  const scipp::index nEdge = state.range(1);
  const scipp::index nHist = 1e7 / nEvent;
  const bool linear = state.range(2);
  const auto events = make_2d_events(nHist, nEvent);
  std::vector<double> edges_(nEdge);
  std::iota(edges_.begin(), edges_.end(), 0.0);
  if (!linear)
    edges_.back() += 0.0001;
  auto edges = makeVariable<double>(Dims{Dim::Y}, Shape{nEdge},
                                    Values(edges_.begin(), edges_.end()));
  edges *= 1000.0 / nEdge * sc_units::one; // ensure all events are in range
  for (auto _ : state) {
    benchmark::DoNotOptimize(histogram(events, edges));
  }
  state.SetItemsProcessed(state.iterations() * nHist * nEvent);
  state.SetBytesProcessed(state.iterations() * nHist *
                          (3 * nEvent + 2 * (nEdge - 1)) * sizeof(double));
  state.counters["const-width-bins"] = linear;
}

// Params are:
// - nEvent
// - nEdge
// - constant-width-bins
BENCHMARK(BM_histogram)
    ->RangeMultiplier(2)
    ->Ranges({{64, 2 << 14}, {128, 2 << 11}, {false, true}});

BENCHMARK_MAIN();
