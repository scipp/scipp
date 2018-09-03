/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <benchmark/benchmark.h>
#include <array>

#include "dataset_view.h"

std::array<gsl::index, 3> getIndex(gsl::index i,
                                   const std::array<gsl::index, 3> &size) {
  std::array<gsl::index, 3> index;
  // i = x + Nx(y + Ny z)
  index[0] = i % size[0];
  index[1] = (i / size[0]) % size[1];
  index[2] = i / (size[0] * size[1]);
  return index;
}

static void BM_index_math(benchmark::State &state) {
  std::array<gsl::index, 3> size{123, 1234, 1245};
  gsl::index volume = size[0] * size[1] * size[2];
  for (auto _ : state) {
    for (int i = 0; i < volume; ++i) {
      benchmark::DoNotOptimize(getIndex(i, size));
    }
  }
  state.SetItemsProcessed(state.iterations() * volume);
}
BENCHMARK(BM_index_math)->UseRealTime();

static void BM_index_math_threaded(benchmark::State &state) {
  std::array<gsl::index, 3> size{123, 1234, 1245};
  gsl::index volume = size[0] * size[1] * size[2];
// Warmup
#pragma omp parallel for num_threads(state.range(0))
  for (int i = 0; i < volume; ++i)
    benchmark::DoNotOptimize(getIndex(i, size));
  for (auto _ : state) {
#pragma omp parallel for num_threads(state.range(0))
    for (int i = 0; i < volume; ++i) {
      benchmark::DoNotOptimize(getIndex(i, size));
    }
  }
  state.SetItemsProcessed(state.iterations() * volume);
}
BENCHMARK(BM_index_math_threaded)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Arg(12)
    ->Arg(24)
    ->UseRealTime();

static void
BM_DatasetView_multi_column_mixed_dimension(benchmark::State &state) {
  Dataset d;
  Dimensions dims;
  dims.add(Dimension::Spectrum, state.range(0));
  d.insert<Data::Int>("specnums", dims, state.range(0));
  dims.add(Dimension::Tof, 1000);
  d.insert<Data::Value>("histograms", dims, state.range(0) * 1000);
  gsl::index elements = 1000 * state.range(0);

  for (auto _ : state) {
    DatasetView<Data::Value, const Data::Int> view(d);
    auto it = view.begin();
    for (int i = 0; i < elements; ++i) {
      benchmark::DoNotOptimize(it->get<Data::Value>());
      it++;
    }
  }
  state.SetItemsProcessed(state.iterations() * elements);
}
BENCHMARK(BM_DatasetView_multi_column_mixed_dimension)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10);

static void BM_DatasetView_mixed_dimension_addition(benchmark::State &state) {
  Dataset d;
  Dimensions dims;
  dims.add(Dimension::Spectrum, state.range(0));
  d.insert<Data::Variance>("", dims, state.range(0));
  dims.add(Dimension::Tof, 100);
  dims.add(Dimension::Run, 10);
  gsl::index elements = state.range(0) * 100 * 10;
  d.insert<Data::Value>("", dims, elements);

  for (auto _ : state) {
    DatasetView<Data::Value, const Data::Variance> view(d);
    const auto end = view.end();
    for (auto it = view.begin(); it != end; ++it) {
      it->get<Data::Value>() -= it->get<Data::Variance>();
    }
  }
  state.SetItemsProcessed(state.iterations() * elements);
  state.SetBytesProcessed(state.iterations() * elements * 3 * sizeof(double));
}
BENCHMARK(BM_DatasetView_mixed_dimension_addition)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 14)
    ->UseRealTime();

static void
BM_DatasetView_mixed_dimension_addition_threaded(benchmark::State &state) {
  Dataset d;
  Dimensions dims;
  dims.add(Dimension::Spectrum, state.range(0));
  d.insert<Data::Variance>("", dims, state.range(0));
  dims.add(Dimension::Tof, 100);
  dims.add(Dimension::Run, 10);
  gsl::index elements = state.range(0) * 100 * 10;
  d.insert<Data::Value>("", dims, elements);

  for (auto _ : state) {
    DatasetView<Data::Value, const Data::Variance> view(d);
    const auto end = view.end();
#pragma omp parallel for num_threads(state.range(1))
    for (auto it = view.begin(); it < end; ++it) {
      it->get<Data::Value>() -= it->get<Data::Variance>();
    }
  }
  state.SetItemsProcessed(state.iterations() * elements);
  state.SetBytesProcessed(state.iterations() * elements * 3 * sizeof(double));
}
BENCHMARK(BM_DatasetView_mixed_dimension_addition_threaded)
    ->RangeMultiplier(2)
    ->Ranges({{8, 8 << 14}, {1, 24}})
    ->UseRealTime();

static void
BM_DatasetView_multi_column_mixed_dimension_nested(benchmark::State &state) {
  gsl::index nSpec = state.range(0);
  Dataset d;
  d.insert<Data::Int>("specnums", {Dimension::Spectrum, nSpec}, nSpec);
  Dimensions dims;
  dims.add(Dimension::Tof, 1000);
  dims.add(Dimension::Spectrum, nSpec);
  d.insert<Data::Value>("histograms", dims, nSpec * 1000);
  d.insert<Data::Variance>("histograms", dims, nSpec * 1000);
  DatasetView<DatasetView<Data::Value>, Data::Int> it(d, {Dimension::Tof});

  for (auto _ : state) {
    DatasetView<DatasetView<Data::Value, Data::Variance>, Data::Int> view(
        d, {Dimension::Tof});
    for (auto &item : view) {
      for (auto &point : item.get<DatasetView<Data::Value, Data::Variance>>()) {
        point.value() -= point.get<Data::Variance>();
      }
    }
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  state.SetBytesProcessed(state.iterations() * nSpec * 1000 * 3 *
                          sizeof(double));
}
BENCHMARK(BM_DatasetView_multi_column_mixed_dimension_nested)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 15);
;

static void BM_DatasetView_multi_column_mixed_dimension_nested_threaded(
    benchmark::State &state) {
  gsl::index nSpec = state.range(0);
  Dataset d;
  d.insert<Data::Int>("specnums", {Dimension::Spectrum, nSpec}, nSpec);
  Dimensions dims;
  dims.add(Dimension::Tof, 1000);
  dims.add(Dimension::Spectrum, nSpec);
  d.insert<Data::Value>("histograms", dims, nSpec * 1000);
  d.insert<Data::Variance>("histograms", dims, nSpec * 1000);
  DatasetView<DatasetView<Data::Value>, Data::Int> it(d, {Dimension::Tof});

  for (auto _ : state) {
    DatasetView<DatasetView<Data::Value, Data::Variance>, Data::Int> view(
        d, {Dimension::Tof});
    const auto end = view.end();
#pragma omp parallel for num_threads(state.range(1))
    for (auto it = view.begin(); it < end; ++it) {
      auto &item = *it;
      for (auto &point : item.get<DatasetView<Data::Value, Data::Variance>>()) {
        point.value() -= point.get<Data::Variance>();
      }
    }
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  state.SetBytesProcessed(state.iterations() * nSpec * 1000 * 3 *
                          sizeof(double));
}
BENCHMARK(BM_DatasetView_multi_column_mixed_dimension_nested_threaded)
    ->RangeMultiplier(2)
    ->Ranges({{8, 8 << 15}, {1, 24}})
    ->UseRealTime();

static void BM_DatasetView_multi_column_mixed_dimension_nested_transpose(
    benchmark::State &state) {
  gsl::index nSpec = state.range(0);
  Dataset d;
  d.insert<Data::Int>("specnums", {Dimension::Spectrum, nSpec}, nSpec);
  Dimensions dims;
  dims.add(Dimension::Spectrum, nSpec);
  dims.add(Dimension::Tof, 1000);
  d.insert<Data::Value>("histograms", dims, nSpec * 1000);
  d.insert<Data::Variance>("histograms", dims, nSpec * 1000);
  DatasetView<DatasetView<Data::Value>, Data::Int> it(d, {Dimension::Tof});

  for (auto _ : state) {
    DatasetView<DatasetView<Data::Value, Data::Variance>, Data::Int> view(
        d, {Dimension::Tof});
    for (auto &item : view) {
      for (auto &point : item.get<DatasetView<Data::Value, Data::Variance>>()) {
        point.value() -= point.get<Data::Variance>();
      }
    }
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  state.SetBytesProcessed(state.iterations() * nSpec * 1000 * 3 *
                          sizeof(double));
}
BENCHMARK(BM_DatasetView_multi_column_mixed_dimension_nested_transpose)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 13);
;

BENCHMARK_MAIN();
