// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include "scipp/dataset/dataset.h"

using namespace scipp;

auto make_table() {
  const scipp::index nRow = 10;
  Dataset d;
  const auto column = makeVariable<double>(Dims{Dim::X}, Shape{nRow});
  d.setData("a", column);
  d.setData("b", column);
  d.setData("c", column);
  return d;
}

static void BM_dataset_create_view(benchmark::State &state) {
  auto d = make_table();
  for (auto _ : state) {
    DatasetView view(d);
  }
  state.SetItemsProcessed(state.iterations());
}

static void BM_dataset_slice(benchmark::State &state) {
  auto d = make_table();
  for (auto _ : state) {
    d.slice({Dim::X, 1});
  }
  state.SetItemsProcessed(state.iterations());
}

static void BM_dataset_slice_item(benchmark::State &state) {
  auto d = make_table();
  for (auto _ : state) {
    d.slice({Dim::X, 1})["b"];
  }
  state.SetItemsProcessed(state.iterations());
}

static void BM_dataset_slice_item_dims(benchmark::State &state) {
  auto d = make_table();
  for (auto _ : state) {
    d.slice({Dim::X, 1})["b"].dims();
  }
  state.SetItemsProcessed(state.iterations());
}

// Benchmark simulating a "real" workload with access to all columns and
// multiple API calls (`dims()` and `data()`).
static void BM_dataset_slice_aggregate(benchmark::State &state) {
  auto d = make_table();
  for (auto _ : state) {
    auto slice = d.slice({Dim::X, 1});
    for (const auto &item : slice) {
      if (!item.dims().contains(Dim::X))
        benchmark::DoNotOptimize(item.data());
    }
  }
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_dataset_create_view);
BENCHMARK(BM_dataset_slice);
BENCHMARK(BM_dataset_slice_item);
BENCHMARK(BM_dataset_slice_item_dims);
BENCHMARK(BM_dataset_slice_aggregate);

BENCHMARK_MAIN();
