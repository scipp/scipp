// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include "scipp/dataset/dataset.h"
#include "scipp/neutron/convert.h"

using namespace scipp;

auto make_beamline(const scipp::index size) {
  Dataset beamline;

  beamline.setCoord(Dim("source-position"),
                    makeVariable<Eigen::Vector3d>(
                        units::m, Values{Eigen::Vector3d{0.0, 0.0, -10.0}}));
  beamline.setCoord(Dim("sample-position"),
                    makeVariable<Eigen::Vector3d>(
                        units::m, Values{Eigen::Vector3d{0.0, 0.0, 0.0}}));

  beamline.setCoord(Dim("position"),
                    makeVariable<Eigen::Vector3d>(Dims{Dim::Spectrum},
                                                  Shape{size}, units::m));
  return beamline;
}

auto make_dense_coord_only(const scipp::index size, const scipp::index count,
                           bool transpose) {
  auto out = make_beamline(size);
  auto var = transpose ? makeVariable<double>(Dims{Dim::Tof, Dim::Spectrum},
                                              Shape{count, size})
                       : makeVariable<double>(Dims{Dim::Spectrum, Dim::Tof},
                                              Shape{size, count});
  out.setCoord(Dim::Tof, std::move(var));
  return out;
}

static void BM_neutron_convert(benchmark::State &state, const Dim targetDim) {
  const scipp::index nBin = state.range(0);
  const scipp::index nHist = 1e8 / nBin;
  const bool transpose = state.range(1);
  const auto dense = make_dense_coord_only(nHist, nBin, transpose);
  for (auto _ : state) {
    state.PauseTiming();
    Dataset data = dense;
    state.ResumeTiming();
    data = neutron::convert(std::move(data), Dim::Tof, targetDim);
    state.PauseTiming();
    data = Dataset();
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * nHist * nBin);
  state.SetBytesProcessed(state.iterations() * nHist * nBin * 2 *
                          sizeof(double));
  state.counters["positions"] = benchmark::Counter(nHist);
  state.counters["transpose"] = transpose;
}

auto make_events_default_weights(const scipp::index size,
                                 const scipp::index count) {
  auto out = make_beamline(size);
  auto var = makeVariable<event_list<double>>(Dims{Dim::Spectrum}, Shape{size});
  auto vals = var.values<event_list<double>>();
  for (scipp::index i = 0; i < size; ++i)
    vals[i].resize(count, 5000.0);
  out.setCoord(Dim::Tof, std::move(var));
  auto weights = makeVariable<double>(Dims{Dim::Spectrum}, Shape{size},
                                      units::counts, Values{}, Variances{});
  out.setData("", weights);
  return out;
}

static void BM_neutron_convert_events(benchmark::State &state,
                                      const Dim targetDim) {
  const scipp::index nEvent = state.range(0);
  const scipp::index nHist = 1e8 / nEvent;
  const auto events = make_events_default_weights(nHist, nEvent);
  for (auto _ : state) {
    state.PauseTiming();
    Dataset data = events;
    state.ResumeTiming();
    data = neutron::convert(std::move(data), Dim::Tof, targetDim);
    state.PauseTiming();
    data = Dataset();
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * nHist * nEvent);
  state.SetBytesProcessed(state.iterations() * nHist * nEvent * 2 *
                          sizeof(double));
  state.counters["positions"] = benchmark::Counter(nHist);
}

// Params are:
// - nBin
// - transpose
BENCHMARK_CAPTURE(BM_neutron_convert, Dim::DSpacing, Dim::DSpacing)
    ->RangeMultiplier(2)
    ->Ranges({{8, 2 << 14}, {false, true}});
BENCHMARK_CAPTURE(BM_neutron_convert, Dim::Wavelength, Dim::Wavelength)
    ->RangeMultiplier(2)
    ->Ranges({{8, 2 << 14}, {false, true}});
BENCHMARK_CAPTURE(BM_neutron_convert, Dim::Energy, Dim::Energy)
    ->RangeMultiplier(2)
    ->Ranges({{8, 2 << 14}, {false, true}});

// Params are:
// - nEvent
BENCHMARK_CAPTURE(BM_neutron_convert_events, Dim::DSpacing, Dim::DSpacing)
    ->RangeMultiplier(2)
    ->Ranges({{8, 2 << 14}});
BENCHMARK_CAPTURE(BM_neutron_convert_events, Dim::Wavelength, Dim::Wavelength)
    ->RangeMultiplier(2)
    ->Ranges({{8, 2 << 14}});
BENCHMARK_CAPTURE(BM_neutron_convert_events, Dim::Energy, Dim::Energy)
    ->RangeMultiplier(2)
    ->Ranges({{8, 2 << 14}});

BENCHMARK_MAIN();
