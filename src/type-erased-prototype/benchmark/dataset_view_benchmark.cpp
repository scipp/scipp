#include <benchmark/benchmark.h>

#include "dataset_view.h"

static void
BM_DatasetView_multi_column_mixed_dimension(benchmark::State &state) {
  Dataset d;
  Dimensions dims;
  dims.add(Dimension::SpectrumNumber, state.range(0));
  d.insert<Variable::Int>("specnums", dims, state.range(0));
  dims.add(Dimension::Tof, 1000);
  d.insert<Variable::Value>("histograms", dims, state.range(0) * 1000);
  gsl::index elements = 1000 * state.range(0);

  for (auto _ : state) {
    DatasetView<Variable::Value, const Variable::Int> it(d);
    for (int i = 0; i < elements; ++i) {
      benchmark::DoNotOptimize(it.get<Variable::Value>());
      it.increment();
    }
  }
  state.SetItemsProcessed(state.iterations() * elements);
}
BENCHMARK(BM_DatasetView_multi_column_mixed_dimension)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10);
;

static void
BM_DatasetView_multi_column_mixed_dimension_slab(benchmark::State &state) {
  Dataset d;
  Dimensions dims;
  dims.add(Dimension::SpectrumNumber, state.range(0));
  d.insert<Variable::Int>("specnums", dims, state.range(0));
  dims.add(Dimension::Tof, 1000);
  d.insert<Variable::Value>("histograms", dims, state.range(0) * 1000);
  DatasetView<Slab<Variable::Value>, Variable::Int> it(d, {Dimension::Tof});
  gsl::index elements = state.range(0);

  for (auto _ : state) {
    DatasetView<Slab<Variable::Value>, Variable::Int> it(d, {Dimension::Tof});
    for (int i = 0; i < elements; ++i) {
      benchmark::DoNotOptimize(it.get<Variable::Int>());
      it.increment();
    }
  }
  state.SetItemsProcessed(state.iterations() * elements);
}
BENCHMARK(BM_DatasetView_multi_column_mixed_dimension_slab)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10);
;

BENCHMARK_MAIN();
