// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
  Variable x = makeRandom(dims, -2.0, 2.0);
  Variable y = makeRandom(dims, -2.0, 2.0);
  return DataArray(data, {{Dim::X, x}, {Dim::Y, y}});
}

auto make_edges(const Dim dim, const scipp::index size) {
  return cumsum(
             broadcast((4.0 / size) * sc_units::one, Dimensions(dim, size + 1)),
             CumSumMode::Exclusive) -
         (2.0 * sc_units::one);
}

static void BM_bin_table(benchmark::State &state) {
  const scipp::index nx = state.range(0);
  const scipp::index nEvent = state.range(1);
  auto table = make_table(nEvent);
  auto edges_x = make_edges(Dim::X, nx);
  auto edges_y = make_edges(Dim::Y, 4);

  for (auto _ : state) {
    // cppcheck-suppress unreadVariable
    auto a = dataset::bin(table, {edges_x, edges_y});
  }
  state.SetItemsProcessed(state.iterations() * nEvent);
  state.counters["xbins"] = nx;
  state.counters["ybins"] = edges_y.dims().volume() - 1;
  state.counters["events"] = nEvent;
}
BENCHMARK(BM_bin_table)
    ->RangeMultiplier(10)
    ->Ranges({{10, 2ul << 19ul}, {2ul << 16ul, 2ul << 15ul}});

static void BM_rebin_outer(benchmark::State &state) {
  const scipp::index nx = state.range(0);
  const scipp::index nEvent = state.range(1);
  auto table = make_table(nEvent);
  auto edges_x = make_edges(Dim::X, nx);
  auto edges_y = make_edges(Dim::Y, 4);

  auto binned = dataset::bin(table, {make_edges(Dim::X, 1e4), edges_y});

  for (auto _ : state) {
    // cppcheck-suppress unreadVariable
    auto a = dataset::bin(binned, {edges_x, edges_y});
  }
  state.SetItemsProcessed(state.iterations() * nEvent);
  state.counters["xbins"] = nx;
  state.counters["ybins"] = edges_y.dims().volume() - 1;
  state.counters["events"] = nEvent;
}
BENCHMARK(BM_rebin_outer)
    ->RangeMultiplier(10)
    ->Ranges({{10, static_cast<int64_t>(1e6)},
              {static_cast<int64_t>(1e5), static_cast<int64_t>(1e8)}});

BENCHMARK_MAIN();
