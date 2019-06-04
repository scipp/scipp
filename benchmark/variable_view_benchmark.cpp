// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include <benchmark/benchmark.h>

#include "variable_view.h"

using namespace scipp::core;

static void BM_ViewIndex(benchmark::State &state) {

  Dimensions dims({{Dim::Y, state.range(0)}, {Dim::X, 2000}});
  std::vector<double> variable(dims.volume());
  VariableView<double> view(variable.data(), 0, dims, dims);

  const auto count = dims.volume();

  for (auto _ : state) {
    double sum = 0.0;
    for ( const auto x : view ) {
      sum += x;
    }
    benchmark::DoNotOptimize(sum);
  }

  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_ViewIndex)->RangeMultiplier(2)->Range(4, 8<<10);;

BENCHMARK_MAIN();
