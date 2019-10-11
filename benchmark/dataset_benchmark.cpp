// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include <numeric>

#include "scipp/core/dataset.h"

// Length of a string that can be stored with SSO
constexpr auto SHORT_STRING_LENGTH = 6;
// Length of a string that cannot take advantage of SSO (i.e. must be allocated)
constexpr auto LONG_STRING_LENGTH = 32;

using namespace scipp;
using namespace scipp::core;

Variable makeCoordData(const Dimensions &dims) {
  std::vector<double> data(dims.volume());
  std::iota(data.begin(), data.end(), 0);
  return makeVariable<double>(dims, data);
}

template <int NameLen> struct Generate2D {
  Dataset operator()(const int size = 100) {
    Dataset d;
    d.setCoord(Dim::X, makeCoordData({Dim::X, size}));
    d.setCoord(Dim::Y, makeCoordData({Dim::Y, size}));
    d.setLabels(std::string(NameLen, 'a'), makeCoordData({Dim::X, size}));
    d.setLabels(std::string(NameLen, 'b'), makeCoordData({Dim::Y, size}));
    return d;
  }
};

template <int NameLen> struct Generate6D {
  Dataset operator()(const int size = 100) {
    Dataset d;
    d.setCoord(Dim::X, makeCoordData({Dim::X, size}));
    d.setCoord(Dim::Y, makeCoordData({Dim::Y, size}));
    d.setCoord(Dim::Z, makeCoordData({Dim::Z, size}));
    d.setCoord(Dim::Qx, makeCoordData({Dim::Qx, size}));
    d.setCoord(Dim::Qy, makeCoordData({Dim::Qy, size}));
    d.setCoord(Dim::Qz, makeCoordData({Dim::Qz, size}));
    d.setLabels(std::string(NameLen, 'a'), makeCoordData({Dim::X, size}));
    d.setLabels(std::string(NameLen, 'b'), makeCoordData({Dim::Y, size}));
    d.setLabels(std::string(NameLen, 'c'), makeCoordData({Dim::Z, size}));
    d.setLabels(std::string(NameLen, 'd'), makeCoordData({Dim::Qx, size}));
    d.setLabels(std::string(NameLen, 'e'), makeCoordData({Dim::Qy, size}));
    d.setLabels(std::string(NameLen, 'f'), makeCoordData({Dim::Qz, size}));
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
    d.labels();
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
    return d.slice({Dim::X, 20, 90}, {Dim::Y, 30, 60});
  }
};

struct SliceXYQz {
  auto operator()(const Dataset &d) {
    return d.slice({Dim::X, 20, 90}, {Dim::Y, 30, 60}, {Dim::Qz, 30, 90});
  }
};

struct SliceZXY {
  auto operator()(const Dataset &d) {
    return d.slice({Dim::Z, 5, 95}, {Dim::X, 20, 90}, {Dim::Y, 30, 60});
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
    s.labels();
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

template <int NameLen> struct GenerateWithDataItems {
  Dataset operator()(const int itemCount = 5, const int size = 100) {
    Dataset d;
    for (auto i = 0; i < itemCount; ++i) {
      d.setData(std::to_string(i) + std::string(NameLen, 'i'),
                makeVariable<double>(
                    {{Dim::X, size}, {Dim::Y, size}, {Dim::Z, size}}));
    }
    return d;
  }
};

template <class Gen>
static void BM_Dataset_item_access(benchmark::State &state) {
  const auto d = Gen()();
  const auto name = d.begin()->second.name();
  for (auto _ : state) {
    d[name];
  }
}
BENCHMARK_TEMPLATE(BM_Dataset_item_access,
                   GenerateWithDataItems<SHORT_STRING_LENGTH>);
BENCHMARK_TEMPLATE(BM_Dataset_item_access,
                   GenerateWithDataItems<LONG_STRING_LENGTH>);

template <class Gen>
static void BM_Dataset_iterate_items(benchmark::State &state) {
  const auto d = Gen()(state.range(0));
  for (auto _ : state) {
    for (const auto &[name, item] : d) {
      benchmark::DoNotOptimize(name);
      benchmark::DoNotOptimize(item);
    }
  }
  state.counters["IterationItemRate"] = benchmark::Counter(
      state.range(0), benchmark::Counter::kIsIterationInvariantRate);
}
BENCHMARK_TEMPLATE(BM_Dataset_iterate_items,
                   GenerateWithDataItems<SHORT_STRING_LENGTH>)
    ->Range(1 << 2, 1 << 8);
BENCHMARK_TEMPLATE(BM_Dataset_iterate_items,
                   GenerateWithDataItems<LONG_STRING_LENGTH>)
    ->Range(1 << 2, 1 << 8);

template <class Gen, class Slice>
static void BM_Dataset_iterate_slice_items(benchmark::State &state) {
  const auto d = Gen()(state.range(0));
  const auto s = Slice()(d);
  for (auto _ : state) {
    for (const auto &[name, item] : s) {
      benchmark::DoNotOptimize(name);
      benchmark::DoNotOptimize(item);
    }
  }
  state.counters["IterationItemRate"] = benchmark::Counter(
      state.range(0), benchmark::Counter::kIsIterationInvariantRate);
}
BENCHMARK_TEMPLATE(BM_Dataset_iterate_slice_items,
                   GenerateWithDataItems<SHORT_STRING_LENGTH>, SliceX)
    ->Range(1 << 2, 1 << 8);
BENCHMARK_TEMPLATE(BM_Dataset_iterate_slice_items,
                   GenerateWithDataItems<SHORT_STRING_LENGTH>, SliceXY)
    ->Range(1 << 2, 1 << 8);
BENCHMARK_TEMPLATE(BM_Dataset_iterate_slice_items,
                   GenerateWithDataItems<SHORT_STRING_LENGTH>, SliceZXY)
    ->Range(1 << 2, 1 << 8);

template <class Gen> static void BM_Dataset_copy(benchmark::State &state) {
  const auto d = Gen()(state.range(0), state.range(1));
  for (auto _ : state) {
    Dataset copy(d);
  }
}
static void Args_Dataset_copy(benchmark::internal::Benchmark *b) {
  b->RangeMultiplier(2)
      ->Ranges({{1, 16}, {32, 64}})
      ->Unit(benchmark::kMicrosecond);
}
BENCHMARK_TEMPLATE(BM_Dataset_copy, GenerateWithDataItems<SHORT_STRING_LENGTH>)
    ->Apply(Args_Dataset_copy);
BENCHMARK_TEMPLATE(BM_Dataset_copy, GenerateWithDataItems<LONG_STRING_LENGTH>)
    ->Apply(Args_Dataset_copy);

BENCHMARK_MAIN();
