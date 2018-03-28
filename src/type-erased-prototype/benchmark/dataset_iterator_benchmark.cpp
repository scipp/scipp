#include <benchmark/benchmark.h>

#include "dataset_iterator.h"

static void
BM_DatasetIterator_multi_column_mixed_dimension(benchmark::State &state) {
  Dataset d;
  d.addColumn<double>("histograms");
  d.addColumn<int>("specnums");
  d.addDimension(Dimension::Tof, 1000);
  d.addDimension(Dimension::SpectrumNumber, state.range(0));
  d.extendAlongDimension(ColumnType::Doubles, Dimension::Tof);
  d.extendAlongDimension(ColumnType::Doubles, Dimension::SpectrumNumber);
  d.extendAlongDimension(ColumnType::Ints, Dimension::SpectrumNumber);
  gsl::index elements = 1000 * state.range(0);

  for (auto _ : state) {
    DatasetIterator<double, const int> it(d);
    for (int i = 0; i < elements; ++i) {
      benchmark::DoNotOptimize(it.get<double>());
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
  d.addColumn<double>("histograms");
  d.addColumn<int>("specnums");
  d.addDimension(Dimension::Tof, 1000);
  d.addDimension(Dimension::SpectrumNumber, state.range(0));
  d.extendAlongDimension(ColumnType::Doubles, Dimension::Tof);
  d.extendAlongDimension(ColumnType::Doubles, Dimension::SpectrumNumber);
  d.extendAlongDimension(ColumnType::Ints, Dimension::SpectrumNumber);
  DatasetIterator<Slab<double>, int> it(d, {Dimension::Tof});
  gsl::index elements = state.range(0);

  for (auto _ : state) {
    DatasetIterator<Slab<double>, int> it(d, {Dimension::Tof});
    for (int i = 0; i < elements; ++i) {
      benchmark::DoNotOptimize(it.get<int>());
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
