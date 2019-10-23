// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#include <numeric>

#include <benchmark/benchmark.h>

#include "scipp/core/groupby.h"

using namespace scipp;
using namespace scipp::core;

auto make_2d_sparse_coord_only(const scipp::index size,
                               const scipp::index count) {
  auto var = makeVariable<double>({Dim::X, Dim::Y}, {size, Dimensions::Sparse});
  auto vals = var.sparseValues<double>();
  for (scipp::index i = 0; i < size; ++i)
    vals[i].resize(count);
  DataArray sparse(std::nullopt, {{Dim::Y, std::move(var)}});
  return sparse;
}

static void BM_groupby_flatten(benchmark::State &state) {
  const scipp::index nEvent = 1e8;
  const scipp::index nHist = state.range(0);
  const scipp::index nGroup = state.range(1);
  auto sparse = make_2d_sparse_coord_only(nHist, nEvent / nHist);
  std::vector<int64_t> group_(nHist);
  std::iota(group_.begin(), group_.end(), 0);
  auto group = makeVariable<int64_t>({Dim::X, nHist}, group_);
  sparse.labels().set("group", group / (nHist / nGroup));
  for (auto _ : state) {
    benchmark::DoNotOptimize(groupby(sparse, "group", Dim::Z).flatten(Dim::X));
  }
  state.SetItemsProcessed(state.iterations() * nEvent);
  // Not taking into account vector reallocations, just the raw "effective" size
  // (read event, write to output).
  state.SetBytesProcessed(state.iterations() * (2 * nEvent) * sizeof(double));
}
// Params are:
// - nHist
// - nGroup
// Also note the special case nHist = nGroup, which should effectively just make
// a copy of the input with reshuffling events.
BENCHMARK(BM_groupby_flatten)
    ->RangeMultiplier(4)
    ->Ranges({{64, 2 << 17}, {1, 64}});

BENCHMARK_MAIN();
