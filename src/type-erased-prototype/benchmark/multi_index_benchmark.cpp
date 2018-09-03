/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <benchmark/benchmark.h>

#include "multi_index.h"

static void BM_MultiIndex(benchmark::State &state) {
  Dimensions dims1;
  dims1.add(Dimension::X, 1000);
  dims1.add(Dimension::Y, 2000);
  dims1.add(Dimension::Z, 3000);
  Dimensions dims2;
  dims2.add(Dimension::Z, 3000);
  dims2.add(Dimension::Y, 2000);
  dims2.add(Dimension::X, 1000);
  const auto count = dims1.volume();

  gsl::index result{0};
  for (auto _ : state) {
    MultiIndex index(dims1, {dims1, dims2});
    for (gsl::index i = 0; i < count; ++i) {
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
