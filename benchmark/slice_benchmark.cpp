// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

auto make_table() {
  const scipp::index nCol = 3;
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
    DatasetProxy view(d);
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

BENCHMARK(BM_dataset_create_view);
BENCHMARK(BM_dataset_slice);

BENCHMARK_MAIN();
