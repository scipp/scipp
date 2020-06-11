// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include <benchmark/benchmark.h>

#include "scipp/core/element_array_view.h"

using namespace scipp::core;

static void BM_ViewIndex(benchmark::State &state) {
  Dimensions dims({{Dim::Y, state.range(0)}, {Dim::X, 2000}});
  std::vector<double> variable(dims.volume());
  ElementArrayView<double> view(variable.data(), 0, dims, dims);

  for (auto _ : state) {
    double sum = 0.0;
    // Caution when iterating over a view!
    // Using a range based loop here is MUCH faster (80x) than using
    //   for ( auto it = view.begin(); it != view.end(); ++it ) {
    //     sum += *it;
    //   }
    // See view_index.h for more details.
    for (const auto x : view) {
      sum += x;
    }
    benchmark::DoNotOptimize(sum);
  }

  state.SetItemsProcessed(state.iterations() * view.size());
}
BENCHMARK(BM_ViewIndex)->RangeMultiplier(2)->Range(4, 8 << 10);
;

BENCHMARK_MAIN();
