// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <numeric>

#include <benchmark/benchmark.h>

#include "random.h"

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

auto make_2d_sparse_coord(const scipp::index size, const scipp::index count) {
  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y},
                                  Shape{size, Dimensions::Sparse});
  auto vals = var.sparseValues<double>();
  Random rand(0.0, 1000.0);
  for (scipp::index i = 0; i < size; ++i) {
    auto data = rand(count);
    vals[i].assign(data.begin(), data.end());
  }
  return var;
}

auto make_2d_sparse_coord_only(const scipp::index size,
                               const scipp::index count) {
  return DataArray(std::nullopt, {{Dim::Y, make_2d_sparse_coord(size, count)}});
}

auto make_2d_sparse(const scipp::index size, const scipp::index count) {
  auto coord = make_2d_sparse_coord(size, count);
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
  const auto sparse = data ? make_2d_sparse(nHist, nEvent)
                           : make_2d_sparse_coord_only(nHist, nEvent);
  std::vector<double> edges_(nEdge);
  std::iota(edges_.begin(), edges_.end(), 0.0);
  if (!linear)
    edges_.back() += 0.0001;
  auto edges = makeVariable<double>(Dims{Dim::Y}, Shape{nEdge},
                                    Values(edges_.begin(), edges_.end()));
  edges *= 1000.0 / nEdge; // ensure all events are in range
  for (auto _ : state) {
    benchmark::DoNotOptimize(histogram(sparse, edges));
  }
  state.SetItemsProcessed(state.iterations() * nHist * nEvent);
  state.SetBytesProcessed(state.iterations() * nHist *
                          ((data ? 3 : 1) * nEvent + 2 * (nEdge - 1)) *
                          sizeof(double));
  state.counters["const-width-bins"] = linear;
  state.counters["sparse-with-data"] = data;
}

// Params are:
// - nEvent
// - nEdge
// - constant-width-bins
// - sparse with data
BENCHMARK(BM_histogram)
    ->RangeMultiplier(2)
    ->Ranges({{64, 2 << 14}, {128, 2 << 11}, {false, true}, {false, true}});

BENCHMARK_MAIN();
