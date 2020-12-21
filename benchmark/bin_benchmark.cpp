// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include "scipp/dataset/bin.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/operations.h"

#include "../test/random.h"

using namespace scipp;

auto make_table(const scipp::index size) {
  Dimensions dims(Dim::Event, size);
  Variable data = makeVariable<double>(Dims{Dim::Event}, Shape{size});
  // Range is -2.0 to 2.0
  Variable x = makeRandom(dims);
  Variable y = makeRandom(dims);
  return DataArray(data, {{Dim::X, x}, {Dim::Y, y}});
}

auto make_edges(const Dim dim, const scipp::index size) {
  return cumsum(broadcast((4.0 / size) * units::one, Dimensions(dim, size + 1)),
                CumSumMode::Exclusive) -
         (2.0 * units::one);
}

static void BM_bin_table(benchmark::State &state) {
  const scipp::index nx = state.range(0);
  const scipp::index nEvent = state.range(1);
  auto table = make_table(nEvent);
  auto edges_x = make_edges(Dim::X, nx);
  auto edges_y = make_edges(Dim::Y, 4);

  for (auto _ : state) {
    auto a = dataset::bin(table, {edges_x, edges_y});
  }
  state.SetItemsProcessed(state.iterations() * nEvent);
  state.counters["xbins"] = nx;
  state.counters["ybins"] = edges_y.dims().volume() - 1;
  state.counters["events"] = nEvent;
}
BENCHMARK(BM_bin_table)->RangeMultiplier(10)->Ranges({{10, 1e6}, {1e5, 1e8}});

BENCHMARK_MAIN();
