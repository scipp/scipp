// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include <numeric>

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

Variable makeCoordData(const Dimensions &dims) {
  std::vector<double> data(dims.volume());
  std::iota(data.begin(), data.end(), 0);
  return makeVariable<double>(dims, data);
}

struct Generate2D {
  Dataset operator()(const int size = 100) {
    Dataset d;
    d.setCoord(Dim::X, makeCoordData({Dim::X, size}));
    d.setCoord(Dim::Y, makeCoordData({Dim::Y, size}));
    return d;
  }
};

struct Generate6D {
  Dataset operator()(const int size = 100) {
    Dataset d;
    d.setCoord(Dim::X, makeCoordData({Dim::X, size}));
    d.setCoord(Dim::Y, makeCoordData({Dim::Y, size}));
    d.setCoord(Dim::Z, makeCoordData({Dim::Z, size}));
    d.setCoord(Dim::Qx, makeCoordData({Dim::Qx, size}));
    d.setCoord(Dim::Qy, makeCoordData({Dim::Qy, size}));
    d.setCoord(Dim::Qz, makeCoordData({Dim::Qz, size}));
    return d;
  }
};

template <class Gen> static void BM_Dataset_coords(benchmark::State &state) {
  const auto d = Gen()();
  for (auto _ : state) {
    d.coords();
  }
}
BENCHMARK_TEMPLATE(BM_Dataset_coords, Generate2D);
BENCHMARK_TEMPLATE(BM_Dataset_coords, Generate6D);

struct SliceX {
  auto operator()(const Dataset &d) { return d.slice({Dim::X, 20, 90}); }
};

struct SliceXY {
  auto operator()(const Dataset &d) {
    return d.slice({Dim::X, 20, 90}, {Dim::Y, 30, 60});
  }
};

struct SliceXYQz {
  auto operator()(const Dataset &d) {
    return d.slice({Dim::X, 20, 90}, {Dim::Y, 30, 60}, {Dim::Qz, 30, 90});
  }
};

template <class Gen, class Slice>
static void BM_Dataset_coords_slice(benchmark::State &state) {
  const auto d = Gen()();
  const auto s = Slice()(d);
  for (auto _ : state) {
    s.coords();
  }
}
BENCHMARK_TEMPLATE(BM_Dataset_coords_slice, Generate2D, SliceX);
BENCHMARK_TEMPLATE(BM_Dataset_coords_slice, Generate2D, SliceXY);
BENCHMARK_TEMPLATE(BM_Dataset_coords_slice, Generate6D, SliceX);
BENCHMARK_TEMPLATE(BM_Dataset_coords_slice, Generate6D, SliceXY);
BENCHMARK_TEMPLATE(BM_Dataset_coords_slice, Generate6D, SliceXYQz);

BENCHMARK_MAIN();
