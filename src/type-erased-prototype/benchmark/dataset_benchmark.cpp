/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <benchmark/benchmark.h>

#include "dataset.h"

// Dataset::get requires a search based on a tag defined by the type and is thus
// potentially expensive.
static void BM_Dataset_get_with_many_columns(benchmark::State &state) {
  Dataset d;
  for (int i = 0; i < state.range(0); ++i)
    d.insert<Data::Value>("name" + std::to_string(i), Dimensions{}, 1);
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

Dataset makeSingleDataDataset(const gsl::index nSpec, const gsl::index nPoint) {
  Dataset d;

  d.insert<Coord::DetectorId>({Dimension::Detector, nSpec}, nSpec);
  d.insert<Coord::DetectorPosition>({Dimension::Detector, nSpec}, nSpec);
  d.insert<Coord::DetectorGrouping>({Dimension::Spectrum, nSpec}, nSpec);
  d.insert<Coord::SpectrumNumber>({Dimension::Spectrum, nSpec}, nSpec);

  d.insert<Coord::Tof>({Dimension::Tof, nPoint}, nPoint);
  Dimensions dims({{Dimension::Tof, nPoint}, {Dimension::Spectrum, nSpec}});
  d.insert<Data::Value>("sample", dims, dims.volume());
  d.insert<Data::Variance>("sample", dims, dims.volume());

  return d;
}

Dataset makeDataset(const gsl::index nSpec, const gsl::index nPoint) {
  Dataset d;

  d.insert<Coord::DetectorId>({Dimension::Detector, nSpec}, nSpec);
  d.insert<Coord::DetectorPosition>({Dimension::Detector, nSpec}, nSpec);
  d.insert<Coord::DetectorGrouping>({Dimension::Spectrum, nSpec}, nSpec);
  d.insert<Coord::SpectrumNumber>({Dimension::Spectrum, nSpec}, nSpec);

  d.insert<Coord::Tof>({Dimension::Tof, nPoint}, nPoint);
  Dimensions dims({{Dimension::Tof, nPoint}, {Dimension::Spectrum, nSpec}});
  d.insert<Data::Value>("sample", dims, dims.volume());
  d.insert<Data::Variance>("sample", dims, dims.volume());
  d.insert<Data::Value>("background", dims, dims.volume());
  d.insert<Data::Variance>("background", dims, dims.volume());

  return d;
}

static void BM_Dataset_plus(benchmark::State &state) {
  gsl::index nSpec = 10000;
  gsl::index nPoint = state.range(0);
  auto d = makeDataset(nSpec, nPoint);
  for (auto _ : state) {
    d += d;
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  // This is the minimal theoretical data volume to and from RAM, loading 2+2,
  // storing 2. That is, this does not take into account intermediate values.
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 6 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_plus)->RangeMultiplier(2)->Range(2 << 9, 2 << 12);

static void BM_Dataset_multiply(benchmark::State &state) {
  gsl::index nSpec = state.range(0);
  gsl::index nPoint = 1024;
  auto d = makeSingleDataDataset(nSpec, nPoint);
  const auto d2 = makeSingleDataDataset(nSpec, nPoint);
  for (auto _ : state) {
    d *= d2;
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  // This is the minimal theoretical data volume to and from RAM, loading 2+2,
  // storing 2. That is, this does not take into account intermediate values.
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 6 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_multiply)->RangeMultiplier(2)->Range(2 << 0, 2 << 12);
BENCHMARK(BM_Dataset_multiply)->RangeMultiplier(2)->Range(2 << 0, 2 << 12)->Threads(4)->UseRealTime();
BENCHMARK(BM_Dataset_multiply)->RangeMultiplier(2)->Range(2 << 0, 2 << 12)->Threads(8)->UseRealTime();
BENCHMARK(BM_Dataset_multiply)->RangeMultiplier(2)->Range(2 << 0, 2 << 12)->Threads(12)->UseRealTime();
BENCHMARK(BM_Dataset_multiply)->RangeMultiplier(2)->Range(2 << 0, 2 << 12)->Threads(24)->UseRealTime();

Dataset doWork(Dataset d) {
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  d *= d;
  return d;
}

static void BM_Dataset_cache_blocking_reference(benchmark::State &state) {
  gsl::index nSpec = 10000;
  gsl::index nPoint = state.range(0);
  auto d = makeDataset(nSpec, nPoint);
  auto out(d);
  for (auto _ : state) {
    d = doWork(std::move(d));
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  // This is the minimal theoretical data volume to and from RAM, loading 2+2,
  // storing 2+2. That is, this does not take into account intermediate values.
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 8 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_cache_blocking_reference)
    ->RangeMultiplier(2)
    ->Range(2 << 9, 2 << 12);

static void BM_Dataset_cache_blocking(benchmark::State &state) {
  gsl::index nSpec = 10000;
  gsl::index nPoint = state.range(0);
  auto d = makeDataset(nSpec, nPoint);
  for (auto _ : state) {
    for (gsl::index i = 0; i < nSpec; ++i) {
      d.setSlice(doWork(slice(d, Dimension::Spectrum, i)), Dimension::Spectrum,
                 i);
    }
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  // This is the minimal theoretical data volume to and from RAM, loading 2+2,
  // storing 2+2. That is, this does not take into account intermediate values.
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 8 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_cache_blocking)
    ->RangeMultiplier(2)
    ->Range(2 << 9, 2 << 14);

static void BM_Dataset_cache_blocking_no_slicing(benchmark::State &state) {
  gsl::index nSpec = 10000;
  gsl::index nPoint = state.range(0);
  const auto d = makeDataset(nSpec, nPoint);
  std::vector<Dataset> slices;
  for (gsl::index i = 0; i < nSpec; ++i) {
    slices.emplace_back(slice(d, Dimension::Spectrum, i));
  }

  for (auto _ : state) {
    for (gsl::index i = 0; i < nSpec; ++i) {
      slices[i] = doWork(std::move(slices[i]));
    }
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  // This is the minimal theoretical data volume to and from RAM, loading 2+2,
  // storing 2+2. That is, this does not take into account intermediate values.
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 8 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_cache_blocking_no_slicing)
    ->RangeMultiplier(2)
    ->Range(2 << 9, 2 << 14);

BENCHMARK_MAIN();
