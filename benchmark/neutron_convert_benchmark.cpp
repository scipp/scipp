// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include "random.h"

#include "scipp/core/dataset.h"
#include "scipp/neutron/convert.h"

using namespace scipp;
using namespace scipp::core;

auto make_beamline(const scipp::index size) {
  Dataset beamline;

  Dataset components;
  // Source and sample
  components.setData("position", makeVariable<Eigen::Vector3d>(
                                     {Dim::Row, 2}, units::m,
                                     {Eigen::Vector3d{0.0, 0.0, -10.0},
                                      Eigen::Vector3d{0.0, 0.0, 0.0}}));
  beamline.setLabels("component_info", makeVariable<Dataset>(components));
  beamline.setLabels("position", makeVariable<Eigen::Vector3d>(
                                     {Dim::Spectrum, size}, units::m));
  return beamline;
}

auto make_sparse_coord_only(const scipp::index size, const scipp::index count) {
  auto var = makeVariable<double>({Dim::Spectrum, Dim::Tof},
                                  {size, Dimensions::Sparse});
  auto vals = var.sparseValues<double>();
  Random rand(0.0, 1000.0);
  for (scipp::index i = 0; i < size; ++i) {
    auto data = rand(count);
    vals[i].assign(data.begin(), data.end());
  }
  DataArray sparse(std::nullopt, {{Dim::Tof, std::move(var)}});
  return merge(make_beamline(size), Dataset({{"", sparse}}));
}

static void BM_neutron_convert(benchmark::State &state) {
  const scipp::index nEvent = state.range(0);
  const scipp::index nHist = 4e7 / nEvent;
  const auto sparse = make_sparse_coord_only(nHist, nEvent);
  for (auto _ : state) {
    benchmark::DoNotOptimize(neutron::convert(sparse, Dim::Tof, Dim::DSpacing));
  }
  state.SetItemsProcessed(state.iterations() * nHist * nEvent);
  state.SetBytesProcessed(state.iterations() * nHist * nEvent * 2 *
                          sizeof(double));
}
// Params are:
// - nEvent
BENCHMARK(BM_neutron_convert)->RangeMultiplier(2)->Ranges({{64, 2 << 14}});

BENCHMARK_MAIN();
