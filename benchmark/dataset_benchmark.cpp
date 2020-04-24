// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include <numeric>

#include "common.h"

#include "scipp/dataset/dataset.h"

// Length of a string that can be stored with SSO
constexpr auto SHORT_STRING_LENGTH = 6;
// Length of a string that cannot take advantage of SSO (i.e. must be allocated)
constexpr auto LONG_STRING_LENGTH = 32;

using namespace scipp;

Variable makeCoordData(const Dimensions &dims) {
  std::vector<double> data(dims.volume());
  std::iota(data.begin(), data.end(), 0);
  return makeVariable<double>(Dimensions(dims),
                              Values(data.begin(), data.end()));
}

template <int NameLen> struct Generate2D {
  Dataset operator()(const int axisLength = 100) {
    Dataset d;
    d.setCoord(Dim::X, makeCoordData({Dim::X, axisLength}));
    d.setCoord(Dim::Y, makeCoordData({Dim::Y, axisLength}));
    d.setCoord(Dim(std::string(NameLen, 'a')),
               makeCoordData({Dim::X, axisLength}));
    d.setCoord(Dim(std::string(NameLen, 'b')),
               makeCoordData({Dim::Y, axisLength}));
    return d;
  }
};

template <int NameLen> struct Generate6D {
  Dataset operator()(const int axisLength = 100) {
    Dataset d;
    d.setCoord(Dim::X, makeCoordData({Dim::X, axisLength}));
    d.setCoord(Dim::Y, makeCoordData({Dim::Y, axisLength}));
    d.setCoord(Dim::Z, makeCoordData({Dim::Z, axisLength}));
    d.setCoord(Dim::Qx, makeCoordData({Dim::Qx, axisLength}));
    d.setCoord(Dim::Qy, makeCoordData({Dim::Qy, axisLength}));
    d.setCoord(Dim::Qz, makeCoordData({Dim::Qz, axisLength}));
    d.setCoord(Dim(std::string(NameLen, 'a')),
               makeCoordData({Dim::X, axisLength}));
    d.setCoord(Dim(std::string(NameLen, 'b')),
               makeCoordData({Dim::Y, axisLength}));
    d.setCoord(Dim(std::string(NameLen, 'c')),
               makeCoordData({Dim::Z, axisLength}));
    d.setCoord(Dim(std::string(NameLen, 'd')),
               makeCoordData({Dim::Qx, axisLength}));
    d.setCoord(Dim(std::string(NameLen, 'e')),
               makeCoordData({Dim::Qy, axisLength}));
    d.setCoord(Dim(std::string(NameLen, 'f')),
               makeCoordData({Dim::Qz, axisLength}));
    return d;
  }
};

template <class Gen> static void BM_Dataset_coords(benchmark::State &state) {
  const auto d = Gen()();
  for (auto _ : state) {
    d.coords();
  }
}
BENCHMARK_TEMPLATE(BM_Dataset_coords, Generate2D<SHORT_STRING_LENGTH>);
BENCHMARK_TEMPLATE(BM_Dataset_coords, Generate6D<SHORT_STRING_LENGTH>);

template <class Gen> static void BM_Dataset_labels(benchmark::State &state) {
  const auto d = Gen()();
  for (auto _ : state) {
    d.coords();
  }
}
BENCHMARK_TEMPLATE(BM_Dataset_labels, Generate2D<SHORT_STRING_LENGTH>);
BENCHMARK_TEMPLATE(BM_Dataset_labels, Generate2D<LONG_STRING_LENGTH>);
BENCHMARK_TEMPLATE(BM_Dataset_labels, Generate6D<SHORT_STRING_LENGTH>);
BENCHMARK_TEMPLATE(BM_Dataset_labels, Generate6D<LONG_STRING_LENGTH>);

struct SliceX {
  auto operator()(const Dataset &d) { return d.slice({Dim::X, 20, 90}); }
};

struct SliceXY {
  auto operator()(const Dataset &d) {
    return d.slice({Dim::X, 20, 90}).slice({Dim::Y, 30, 60});
  }
};

struct SliceXYQz {
  auto operator()(const Dataset &d) {
    return d.slice({Dim::X, 20, 90})
        .slice({Dim::Y, 30, 60})
        .slice({Dim::Qz, 30, 90});
  }
};

struct SliceZXY {
  auto operator()(const Dataset &d) {
    return d.slice({Dim::Z, 5, 95})
        .slice({Dim::X, 20, 90})
        .slice({Dim::Y, 30, 60});
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
BENCHMARK_TEMPLATE(BM_Dataset_coords_slice, Generate2D<SHORT_STRING_LENGTH>,
                   SliceX);
BENCHMARK_TEMPLATE(BM_Dataset_coords_slice, Generate2D<SHORT_STRING_LENGTH>,
                   SliceXY);
BENCHMARK_TEMPLATE(BM_Dataset_coords_slice, Generate6D<SHORT_STRING_LENGTH>,
                   SliceX);
BENCHMARK_TEMPLATE(BM_Dataset_coords_slice, Generate6D<SHORT_STRING_LENGTH>,
                   SliceXY);
BENCHMARK_TEMPLATE(BM_Dataset_coords_slice, Generate6D<SHORT_STRING_LENGTH>,
                   SliceXYQz);

template <class Gen, class Slice>
static void BM_Dataset_labels_slice(benchmark::State &state) {
  const auto d = Gen()();
  const auto s = Slice()(d);
  for (auto _ : state) {
    s.coords();
  }
}
BENCHMARK_TEMPLATE(BM_Dataset_labels_slice, Generate2D<SHORT_STRING_LENGTH>,
                   SliceX);
BENCHMARK_TEMPLATE(BM_Dataset_labels_slice, Generate2D<LONG_STRING_LENGTH>,
                   SliceX);
BENCHMARK_TEMPLATE(BM_Dataset_labels_slice, Generate2D<SHORT_STRING_LENGTH>,
                   SliceXY);
BENCHMARK_TEMPLATE(BM_Dataset_labels_slice, Generate2D<LONG_STRING_LENGTH>,
                   SliceXY);
BENCHMARK_TEMPLATE(BM_Dataset_labels_slice, Generate6D<SHORT_STRING_LENGTH>,
                   SliceX);
BENCHMARK_TEMPLATE(BM_Dataset_labels_slice, Generate6D<LONG_STRING_LENGTH>,
                   SliceX);
BENCHMARK_TEMPLATE(BM_Dataset_labels_slice, Generate6D<SHORT_STRING_LENGTH>,
                   SliceXY);
BENCHMARK_TEMPLATE(BM_Dataset_labels_slice, Generate6D<LONG_STRING_LENGTH>,
                   SliceXY);
BENCHMARK_TEMPLATE(BM_Dataset_labels_slice, Generate6D<SHORT_STRING_LENGTH>,
                   SliceXYQz);
BENCHMARK_TEMPLATE(BM_Dataset_labels_slice, Generate6D<LONG_STRING_LENGTH>,
                   SliceXYQz);

template <class Gen>
static void BM_Dataset_item_access(benchmark::State &state) {
  const auto d = std::get<0>(Gen()());
  const auto name = d.begin()->name();
  for (auto _ : state) {
    d[name];
  }
}
BENCHMARK_TEMPLATE(BM_Dataset_item_access,
                   Generate3DWithDataItems<SHORT_STRING_LENGTH>);
BENCHMARK_TEMPLATE(BM_Dataset_item_access,
                   Generate3DWithDataItems<LONG_STRING_LENGTH>);

template <class Gen>
static void BM_Dataset_iterate_items(benchmark::State &state) {
  const auto itemCount = state.range(0);
  const auto d = std::get<0>(Gen()(itemCount));
  for (auto _ : state) {
    for (const auto &item : d) {
      benchmark::DoNotOptimize(item);
    }
  }
  state.counters["IterationItemRate"] = benchmark::Counter(
      itemCount, benchmark::Counter::kIsIterationInvariantRate);
}
BENCHMARK_TEMPLATE(BM_Dataset_iterate_items,
                   Generate3DWithDataItems<SHORT_STRING_LENGTH>)
    ->Range(1 << 2, 1 << 8);
BENCHMARK_TEMPLATE(BM_Dataset_iterate_items,
                   Generate3DWithDataItems<LONG_STRING_LENGTH>)
    ->Range(1 << 2, 1 << 8);

template <class Gen, class Slice>
static void BM_Dataset_iterate_slice_items(benchmark::State &state) {
  const auto itemCount = state.range(0);
  const auto d = std::get<0>(Gen()(itemCount));
  const auto s = Slice()(d);
  for (auto _ : state) {
    for (const auto &item : s) {
      benchmark::DoNotOptimize(item);
    }
  }
  state.counters["IterationItemRate"] = benchmark::Counter(
      itemCount, benchmark::Counter::kIsIterationInvariantRate);
}
BENCHMARK_TEMPLATE(BM_Dataset_iterate_slice_items,
                   Generate3DWithDataItems<SHORT_STRING_LENGTH>, SliceX)
    ->Range(1 << 2, 1 << 8);
BENCHMARK_TEMPLATE(BM_Dataset_iterate_slice_items,
                   Generate3DWithDataItems<SHORT_STRING_LENGTH>, SliceXY)
    ->Range(1 << 2, 1 << 8);
BENCHMARK_TEMPLATE(BM_Dataset_iterate_slice_items,
                   Generate3DWithDataItems<SHORT_STRING_LENGTH>, SliceZXY)
    ->Range(1 << 2, 1 << 8);

template <class Gen> static void BM_Dataset_copy(benchmark::State &state) {
  const auto itemCount = state.range(0);
  const auto itemLength = state.range(1);
  const auto [d, size] = Gen()(itemCount, itemLength);
  for (auto _ : state) {
    Dataset copy(d);
  }
  state.counters["SizeBytes"] = size;
  state.counters["BandwidthBytes"] =
      benchmark::Counter(size, benchmark::Counter::kIsIterationInvariantRate,
                         benchmark::Counter::OneK::kIs1024);
}
static void Args_Dataset_copy(benchmark::internal::Benchmark *b) {
  b->RangeMultiplier(2)
      ->Ranges({/* Item count */ {1, 16},
                /* Axis length */ {32, 64}})
      ->Unit(benchmark::kMicrosecond);
}
BENCHMARK_TEMPLATE(BM_Dataset_copy,
                   Generate3DWithDataItems<SHORT_STRING_LENGTH>)
    ->Apply(Args_Dataset_copy);
BENCHMARK_TEMPLATE(BM_Dataset_copy, Generate3DWithDataItems<LONG_STRING_LENGTH>)
    ->Apply(Args_Dataset_copy);
BENCHMARK_TEMPLATE(BM_Dataset_copy,
                   GenerateWithEventsDataItems<SHORT_STRING_LENGTH>)
    ->Apply(Args_Dataset_copy);
BENCHMARK_TEMPLATE(BM_Dataset_copy,
                   GenerateWithEventsDataItems<LONG_STRING_LENGTH>)
    ->Apply(Args_Dataset_copy);

static void BM_Dataset_setData_replace(benchmark::State &state) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{1});
  Dataset d;
  d.setData("x", var);
  for (auto _ : state) {
    d.setData("x", var);
  }
}
BENCHMARK(BM_Dataset_setData_replace);

BENCHMARK_MAIN();
