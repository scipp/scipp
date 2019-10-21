// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#include <numeric>

#include <benchmark/benchmark.h>

#include "random.h"

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

auto make_2d_sparse_coord_only(const scipp::index size,
                               const scipp::index count) {
  auto var = makeVariable<double>({Dim::X, Dim::Y}, {size, Dimensions::Sparse});
  auto vals = var.sparseValues<double>();
  Random rand(0.0, 1000.0);
  for (scipp::index i = 0; i < size; ++i) {
    auto data = rand(count);
    vals[i].assign(data.begin(), data.end());
  }
  DataArray sparse(std::nullopt, {{Dim::Y, std::move(var)}});
  return sparse;
}

static void BM_histogram(benchmark::State &state) {
  const scipp::index nHist = 1000;
  const scipp::index nEvent = 1000;
  const scipp::index nEdge = 1000;
  const auto sparse = make_2d_sparse_coord_only(nHist, nEvent);
  std::vector<double> edges_(nEdge);
  std::iota(edges_.begin(), edges_.end(), 0.0);
  auto edges = makeVariable<double>({Dim::Y, nEdge}, edges_);
  edges *= 1000.0 / nEdge; // ensure all events are in range
  for (auto _ : state) {
    benchmark::DoNotOptimize(histogram(sparse, edges));
  }
  state.SetItemsProcessed(state.iterations() * nHist * nEvent);
  state.SetBytesProcessed(state.iterations() * nHist *
                          (nEvent + 2 * (nEdge - 1)) * sizeof(double));
}
BENCHMARK(BM_histogram);

BENCHMARK_MAIN();
