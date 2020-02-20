// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <benchmark/benchmark.h>

#include "multi_index.h"

using namespace scipp::core;

static void BM_MultiIndex(benchmark::State &state) {
  Dimensions dims1;
  dims1.add(Dim::X, 1000);
  dims1.add(Dim::Y, 2000);
  dims1.add(Dim::Z, 3000);
  Dimensions dims2;
  dims2.add(Dim::Z, 3000);
  dims2.add(Dim::Y, 2000);
  dims2.add(Dim::X, 1000);
  const auto count = dims1.volume();

  scipp::index result{0};
  for (auto _ : state) {
    MultiIndex index(dims1, {dims1, dims2});
    for (scipp::index i = 0; i < count; ++i) {
      // benchmark::DoNotOptimize(index.get<0>()) leads to inefficient code (2x
      // slower), we need to actually use the index;
      result -= index.get<0>();
      result -= index.get<1>();
      index.increment();
    }
  }
  printf("%ld\n", result);
  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_MultiIndex);

BENCHMARK_MAIN();
