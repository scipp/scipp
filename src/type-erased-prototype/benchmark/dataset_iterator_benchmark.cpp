#include <benchmark/benchmark.h>

#include "dataset_iterator.h"

static void
BM_DatasetIterator_multi_column_mixed_dimension(benchmark::State &state) {
  Dataset d;
  d.add<Variable::Value>("histograms");
  d.add<Value::Int>("specnums");
  d.addDimension(Dimension::Tof, 1000);
  d.addDimension(Dimension::SpectrumNumber, state.range(0));
  d.extendAlongDimension<Variable::Value>(Dimension::Tof);
  d.extendAlongDimension<Variable::Value>(Dimension::SpectrumNumber);
  d.extendAlongDimension<Variable::Int>(Dimension::SpectrumNumber);
  gsl::index elements = 1000 * state.range(0);

  for (auto _ : state) {
    DatasetIterator<Variable::Value, const Value::Int> it(d);
    for (int i = 0; i < elements; ++i) {
      benchmark::DoNotOptimize(it.get<Variable::Value>());
      it.increment();
    }
  }
  state.SetItemsProcessed(state.iterations() * elements);
}
BENCHMARK(BM_DatasetIterator_multi_column_mixed_dimension)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10);
;

static void
BM_DatasetIterator_multi_column_mixed_dimension_slab(benchmark::State &state) {
  Dataset d;
  d.add<Variable::Value>("histograms");
  d.add<Value::Int>("specnums");
  d.addDimension(Dimension::Tof, 1000);
  d.addDimension(Dimension::SpectrumNumber, state.range(0));
  d.extendAlongDimension<Variable::Value>(Dimension::Tof);
  d.extendAlongDimension<Variable::Value>(Dimension::SpectrumNumber);
  d.extendAlongDimension<Variable::Int>(Dimension::SpectrumNumber);
  DatasetIterator<Slab<Variable::Value>, Value::Int> it(d, {Dimension::Tof});
  gsl::index elements = state.range(0);

  for (auto _ : state) {
    DatasetIterator<Slab<Variable::Value>, Value::Int> it(d, {Dimension::Tof});
    for (int i = 0; i < elements; ++i) {
      benchmark::DoNotOptimize(it.get<Value::Int>());
      it.increment();
    }
  }
  state.SetItemsProcessed(state.iterations() * elements);
}
BENCHMARK(BM_DatasetIterator_multi_column_mixed_dimension_slab)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10);
;

BENCHMARK_MAIN();
