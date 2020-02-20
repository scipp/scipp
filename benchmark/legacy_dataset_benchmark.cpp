// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <benchmark/benchmark.h>

#include <numeric>
#include <random>

#include "dataset.h"

using namespace scipp::core;

// Dataset::get requires a search based on a tag defined by the type and is thus
// potentially expensive.
static void BM_Dataset_get_with_many_columns(benchmark::State &state) {
  Dataset d;
  for (int i = 0; i < state.range(0); ++i)
    d.insert(Data::Value, "name" + std::to_string(i), Dimensions{}, 1);
  d.insert(Data::Variance, "name", Dimensions{}, 1);
  for (auto _ : state)
    d.get(Data::Variance);
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_get_with_many_columns)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10);

// Benchmark demonstrating a potential use of Dataset to replace Histogram. What
// are the performance implications?
static void BM_Dataset_as_Histogram(benchmark::State &state) {
  scipp::index nPoint = state.range(0);
  Dataset d;
  d.insert(Coord::Tof, {Dim::Tof, nPoint}, nPoint);
  d.insert(Data::Value, "", {Dim::Tof, nPoint}, nPoint);
  d.insert(Data::Variance, "", {Dim::Tof, nPoint}, nPoint);
  std::vector<Dataset> histograms;
  scipp::index nSpec = std::min(1000000l, 10000000 / (nPoint + 1));
  for (scipp::index i = 0; i < nSpec; ++i) {
    auto hist(d);
    // Break sharing
    hist.get(Data::Value);
    hist.get(Data::Variance);
    histograms.push_back(hist);
  }

  for (auto _ : state) {
    auto sum = histograms[0];
    for (scipp::index i = 1; i < nSpec; ++i)
      sum += histograms[i];
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  state.SetBytesProcessed(state.iterations() * nSpec * nPoint * 2 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_as_Histogram)->RangeMultiplier(2)->Range(0, 2 << 14);

static void BM_Dataset_as_Histogram_with_slice(benchmark::State &state) {
  Dataset d;
  d.insert(Coord::Tof, {Dim::Tof, 1000}, 1000);
  scipp::index nSpec = 10000;
  Dimensions dims({{Dim::Tof, 1000}, {Dim::Spectrum, nSpec}});
  d.insert(Data::Value, "sample", dims, dims.volume());
  d.insert(Data::Variance, "sample", dims, dims.volume());

  for (auto _ : state) {
    auto sum = d(Dim::Spectrum, 0);
    for (scipp::index i = 1; i < nSpec; ++i)
      sum += d(Dim::Spectrum, i);
  }
  state.SetItemsProcessed(state.iterations() * nSpec);
  state.SetBytesProcessed(state.iterations() * nSpec * 1000 * 2 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_as_Histogram_with_slice);

Dataset makeSingleDataDataset(const scipp::index nSpec,
                              const scipp::index nPoint) {
  Dataset d;

  d.insert(Coord::DetectorId, {Dim::Detector, nSpec}, nSpec);
  d.insert(Coord::Position, {Dim::Detector, nSpec}, nSpec);
  d.insert(Coord::DetectorGrouping, {Dim::Spectrum, nSpec}, nSpec);
  d.insert(Coord::SpectrumNumber, {Dim::Spectrum, nSpec}, nSpec);

  d.insert(Coord::Tof, {Dim::Tof, nPoint}, nPoint);
  Dimensions dims({{Dim::Tof, nPoint}, {Dim::Spectrum, nSpec}});
  d.insert(Data::Value, "sample", dims, dims.volume());
  d.insert(Data::Variance, "sample", dims, dims.volume());

  return d;
}

Dataset makeDataset(const scipp::index nSpec, const scipp::index nPoint) {
  Dataset d;

  d.insert(Coord::DetectorId, {Dim::Detector, nSpec}, nSpec);
  d.insert(Coord::Position, {Dim::Detector, nSpec}, nSpec);
  d.insert(Coord::DetectorGrouping, {Dim::Spectrum, nSpec}, nSpec);
  d.insert(Coord::SpectrumNumber, {Dim::Spectrum, nSpec}, nSpec);

  d.insert(Coord::Tof, {Dim::Tof, nPoint}, nPoint);
  Dimensions dims({{Dim::Tof, nPoint}, {Dim::Spectrum, nSpec}});
  d.insert(Data::Value, "sample", dims, dims.volume());
  d.insert(Data::Variance, "sample", dims, dims.volume());
  d.insert(Data::Value, "background", dims, dims.volume());
  d.insert(Data::Variance, "background", dims, dims.volume());

  return d;
}

static void BM_Dataset_plus(benchmark::State &state) {
  scipp::index nSpec = 10000;
  scipp::index nPoint = state.range(0);
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
  scipp::index nSpec = state.range(0);
  scipp::index nPoint = 1024;
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
BENCHMARK(BM_Dataset_multiply)
    ->RangeMultiplier(2)
    ->Range(2 << 0, 2 << 12)
    ->Threads(4)
    ->UseRealTime();
BENCHMARK(BM_Dataset_multiply)
    ->RangeMultiplier(2)
    ->Range(2 << 0, 2 << 12)
    ->Threads(8)
    ->UseRealTime();
BENCHMARK(BM_Dataset_multiply)
    ->RangeMultiplier(2)
    ->Range(2 << 0, 2 << 12)
    ->Threads(12)
    ->UseRealTime();
BENCHMARK(BM_Dataset_multiply)
    ->RangeMultiplier(2)
    ->Range(2 << 0, 2 << 12)
    ->Threads(24)
    ->UseRealTime();

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
  scipp::index nSpec = 10000;
  scipp::index nPoint = state.range(0);
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
  scipp::index nSpec = 10000;
  scipp::index nPoint = state.range(0);
  auto d = makeDataset(nSpec, nPoint);
  for (auto _ : state) {
    for (scipp::index i = 0; i < nSpec; ++i) {
      d(Dim::Spectrum, i).assign(doWork(d(Dim::Spectrum, i)));
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
  scipp::index nSpec = 10000;
  scipp::index nPoint = state.range(0);
  const auto d = makeDataset(nSpec, nPoint);
  std::vector<Dataset> slices;
  for (scipp::index i = 0; i < nSpec; ++i) {
    slices.emplace_back(d(Dim::Spectrum, i));
  }

  for (auto _ : state) {
    for (scipp::index i = 0; i < nSpec; ++i) {
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

Dataset makeBeamline(const scipp::index nDet) {
  // TODO The created beamline is currently very incomplete so the full cost
  // would be higher.
  Dataset dets;

  dets.insert(Coord::DetectorId, {Dim::Detector, nDet});
  dets.insert(Coord::Mask, {Dim::Detector, nDet});
  dets.insert(Coord::Position, {Dim::Detector, nDet});
  auto ids = dets.get(Coord::DetectorId);
  std::iota(ids.begin(), ids.end(), 1);

  Dataset d;
  d.insert(Coord::DetectorInfo, {}, {dets});
  d.insert(Attr::ExperimentLog, "NeXus logs", {});

  return d;
}

Dataset makeSpectra(const scipp::index nSpec) {
  Dataset d;

  d.insert(Coord::DetectorGrouping, {Dim::Spectrum, nSpec});
  d.insert(Coord::SpectrumNumber, {Dim::Spectrum, nSpec});
  auto groups = d.get(Coord::DetectorGrouping);
  for (scipp::index i = 0; i < groups.size(); ++i)
    groups[i] = {i};
  auto nums = d.get(Coord::SpectrumNumber);
  std::iota(nums.begin(), nums.end(), 1);

  return d;
}

Dataset makeData(const scipp::index nSpec, const scipp::index nPoint) {
  Dataset d;
  d.insert(Coord::Tof, {Dim::Tof, nPoint + 1});
  auto tofs = d.get(Coord::Tof);
  std::iota(tofs.begin(), tofs.end(), 0.0);
  Dimensions dims({{Dim::Tof, nPoint}, {Dim::Spectrum, nSpec}});
  d.insert(Data::Value, "sample", dims);
  d.insert(Data::Variance, "sample", dims);
  return d;
}

Dataset makeWorkspace2D(const scipp::index nSpec, const scipp::index nPoint) {
  auto d = makeBeamline(nSpec);
  d.merge(makeSpectra(nSpec));
  d.merge(makeData(nSpec, nPoint));
  return d;
}

static void BM_Dataset_Workspace2D_create(benchmark::State &state) {
  scipp::index nSpec = 1024 * 1024;
  scipp::index nPoint = 2;
  for (auto _ : state) {
    auto d = makeWorkspace2D(nSpec, nPoint);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_Workspace2D_create);

static void BM_Dataset_Workspace2D_copy(benchmark::State &state) {
  scipp::index nSpec = 1024 * 1024;
  scipp::index nPoint = 2;
  auto d = makeWorkspace2D(nSpec, nPoint);
  for (auto _ : state) {
    auto copy(d);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_Workspace2D_copy);

static void BM_Dataset_Workspace2D_copy_and_write(benchmark::State &state) {
  scipp::index nSpec = 1024 * 1024;
  scipp::index nPoint = state.range(0);
  auto d = makeWorkspace2D(nSpec, nPoint);
  for (auto _ : state) {
    auto copy(d);
    copy.get(Data::Value)[0] = 1.0;
    copy.get(Data::Variance)[0] = 1.0;
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_Workspace2D_copy_and_write)
    ->RangeMultiplier(2)
    ->Range(2, 2 << 7);

static void BM_Dataset_Workspace2D_rebin(benchmark::State &state) {
  scipp::index nSpec = state.range(0) * 1024;
  scipp::index nPoint = 1024;

  auto newCoord = makeVariable<double>({Dim::Tof, nPoint / 2});
  double value = 0.0;
  for (auto &tof : newCoord.span<double>()) {
    tof = value;
    value += 3.0;
  }

  for (auto _ : state) {
    state.PauseTiming();
    auto d = makeData(nSpec, nPoint);
    state.ResumeTiming();
    auto rebinned = rebin(d, newCoord);
  }
  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * nSpec * (nPoint + nPoint / 2) *
                          (1 + 1) * sizeof(double));
}
BENCHMARK(BM_Dataset_Workspace2D_rebin)
    ->RangeMultiplier(2)
    ->Range(32, 1024)
    ->UseRealTime();

Dataset makeEventWorkspace(const scipp::index nSpec,
                           const scipp::index nEvent) {
  auto d = makeBeamline(nSpec);
  d.merge(makeSpectra(nSpec));

  d.insert(Coord::Tof, {Dim::Tof, 2});

  d.insert(Data::Events, "events", {Dim::Spectrum, nSpec});
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist(0, nEvent);
  Dataset empty;
  empty.insert(Data::Tof, "", {Dim::Event, 0});
  empty.insert(Data::PulseTime, "", {Dim::Event, 0});
  for (auto &eventList : d.get(Data::Events)) {
    // 1/4 of the event lists is empty
    scipp::index count = std::max(0l, dist(mt) - nEvent / 4);
    if (count == 0)
      eventList = empty;
    else {
      eventList.insert(Data::Tof, "", {Dim::Event, count});
      eventList.insert(Data::PulseTime, "", {Dim::Event, count});
    }
  }

  return d;
}

static void BM_Dataset_EventWorkspace_create(benchmark::State &state) {
  scipp::index nSpec = 1024 * 1024;
  scipp::index nEvent = 0;
  for (auto _ : state) {
    auto d = makeEventWorkspace(nSpec, nEvent);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_EventWorkspace_create);

static void BM_Dataset_EventWorkspace_copy(benchmark::State &state) {
  scipp::index nSpec = 1024 * 1024;
  scipp::index nEvent = 0;
  auto d = makeEventWorkspace(nSpec, nEvent);
  for (auto _ : state) {
    auto copy(d);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_EventWorkspace_copy);

static void BM_Dataset_EventWorkspace_copy_and_write(benchmark::State &state) {
  scipp::index nSpec = 1024 * 1024;
  scipp::index nEvent = state.range(0);
  auto d = makeEventWorkspace(nSpec, nEvent);
  for (auto _ : state) {
    auto copy(d);
    auto eventLists = copy.get(Data::Events);
    static_cast<void>(eventLists);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Dataset_EventWorkspace_copy_and_write)
    ->RangeMultiplier(8)
    ->Range(2, 2 << 10);

static void BM_Dataset_EventWorkspace_plus(benchmark::State &state) {
  scipp::index nSpec = 128 * 1024;
  scipp::index nEvent = state.range(0);
  auto d = makeEventWorkspace(nSpec, nEvent);
  for (auto _ : state) {
    auto sum = d + d;
  }

  scipp::index actualEvents = 0;
  for (auto &eventList : d.get(Data::Events))
    actualEvents += eventList.dimensions()[Dim::Event];
  state.SetItemsProcessed(state.iterations());
  // 2 for Tof and PulseTime
  // 1+1+2+2 for loads and save
  state.SetBytesProcessed(state.iterations() * actualEvents * 2 * 6 *
                          sizeof(double));
}
BENCHMARK(BM_Dataset_EventWorkspace_plus)
    ->RangeMultiplier(2)
    ->Range(2, 2 << 12);

static void BM_Dataset_EventWorkspace_grow(benchmark::State &state) {
  scipp::index nSpec = 128 * 1024;
  scipp::index nEvent = state.range(0);
  auto d = makeEventWorkspace(nSpec, nEvent);
  auto update = makeEventWorkspace(nSpec, 100);
  for (auto _ : state) {
    state.PauseTiming();
    auto sum(d);
    static_cast<void>(sum.get(Data::Events));
    state.ResumeTiming();
    sum += update;
  }

  scipp::index actualEvents = 0;
  for (auto &eventList : update.get(Data::Events))
    actualEvents += eventList.dimensions()[Dim::Event];
  state.SetItemsProcessed(state.iterations() * actualEvents);
}
BENCHMARK(BM_Dataset_EventWorkspace_grow)
    ->RangeMultiplier(2)
    ->Range(2, 2 << 13);

BENCHMARK_MAIN();
