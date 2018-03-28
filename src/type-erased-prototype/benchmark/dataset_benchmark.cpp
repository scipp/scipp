#include <benchmark/benchmark.h>

#include "dataset.h"

// Dataset::get requires a search based on a tag defined by the type and is thus
// potentially expensive.
static void BM_Dataset_get_with_many_columns(benchmark::State& state) {
  Dataset d;
  for (int i = 0; i < state.range(0); ++i)
    d.addColumn<double>("name" + i);
  d.addColumn<int>("name");
  for (auto _ : state)
    d.get<Ints>();
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_get_with_many_columns)->RangeMultiplier(2)->Range(8, 8<<10);;

BENCHMARK_MAIN();
