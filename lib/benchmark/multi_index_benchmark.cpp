// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <iostream>

#include <benchmark/benchmark.h>

#include "scipp/core/multi_index.h"

using namespace scipp;
using namespace scipp::core;

static void BM_MultiIndex(benchmark::State &state) {
  Dimensions dims;
  dims.add(Dim::X, 1000);
  dims.add(Dim::Y, 2000);
  dims.add(Dim::Z, 3000);
  Strides strides{1, 1000, 1000 * 2000};
  const auto count = dims.volume();

  scipp::index result{0};
  for (auto _ : state) {
    MultiIndex index(dims, strides, strides);
    for (scipp::index i = 0; i < count; ++i) {
      // benchmark::DoNotOptimize(index.get<0>()) leads to inefficient code (2x
      // slower), we need to actually use the index;
      result -= index.get()[0];
      result -= index.get()[1];
      index.increment();
    }
  }
  std::cout << result << '\n';
  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_MultiIndex);

BENCHMARK_MAIN();
