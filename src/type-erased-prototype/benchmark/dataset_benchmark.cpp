#include <benchmark/benchmark.h>

#include "dataset.h"

// Dataset::get requires a search based on a tag defined by the type and is thus
// potentially expensive.
static void BM_Dataset_get_with_many_columns(benchmark::State &state) {
  Dataset d;
  for (int i = 0; i < state.range(0); ++i)
    d.insert<Data::Value>("name" + i, Dimensions{}, 1);
  d.insert<Data::Int>("name", Dimensions{}, 1);
  for (auto _ : state)
    d.get<Data::Int>();
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_get_with_many_columns)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10);

// Benchmark demonstrating a potential use of Dataset to replace Histogram. What
// are the performance implications?
static void BM_Dataset_as_Histogram(benchmark::State &state) {
  gsl::index nPoint = state.range(0);
  Dataset d;
  d.insert<Coord::Tof>({Dimension::Tof, nPoint}, nPoint);
  d.insert<Data::Value>("sample", {Dimension::Tof, nPoint}, nPoint);
  d.insert<Data::Variance>("sample", {Dimension::Tof, nPoint}, nPoint);
  std::vector<Dataset> histograms;
  gsl::index nSpec = std::min(1000000l, 10000000 / nPoint);
  for (gsl::index i = 0; i < nSpec; ++i) {
    auto hist(d);
    // Break sharing
    hist.get<Data::Value>()[0] = 1.0;
    hist.get<Data::Variance>()[0] = 1.0;
    histograms.push_back(hist);
  }

  for (auto _ : state) {
    auto sum = histograms[0];
    for (gsl::index i = 1; i < nSpec; ++i)
      sum += histograms[i];
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 2 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_as_Histogram)->RangeMultiplier(2)->Range(1, 2 << 14);

static void BM_Dataset_as_Histogram_with_slice(benchmark::State &state) {
  Dataset d;
  d.insert<Coord::Tof>({Dimension::Tof, 1000}, 1000);
  gsl::index nSpec = 10000;
  Dimensions dims({{Dimension::Tof, 1000}, {Dimension::Spectrum, nSpec}});
  d.insert<Data::Value>("sample", dims, dims.volume());
  d.insert<Data::Variance>("sample", dims, dims.volume());

  for (auto _ : state) {
    auto sum = slice(d, Dimension::Spectrum, 0);
    for (gsl::index i = 1; i < nSpec; ++i)
      sum += slice(d, Dimension::Spectrum, i);
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  state.SetBytesProcessed(state.iterations() * nSpec * 1000 * 2 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_as_Histogram_with_slice);

BENCHMARK_MAIN();
