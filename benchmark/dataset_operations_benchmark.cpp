// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include <numeric>

#include "common.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/reduction.h"

using namespace scipp;
using namespace scipp::core;

std::vector<bool> make_bools(const scipp::index size,
                             std::initializer_list<bool> pattern) {
  std::vector<bool> result(size);
  auto it = pattern.begin();
  for (auto &&itm : result) {
    if (it == pattern.end())
      it = pattern.begin();
    itm = *(it++);
  }
  return result;
}
std::vector<bool> make_bools(const scipp::index size, bool pattern) {
  return make_bools(size, std::initializer_list<bool>{pattern});
}

template <typename DType> Variable makeData(const Dimensions &dims) {
  std::vector<DType> data(dims.volume());
  std::iota(data.begin(), data.end(), static_cast<DType>(0));
  return makeVariable<DType>(Dimensions(dims),
                             Values(data.begin(), data.end()));
}
struct Generate {
  Dataset operator()(const int axisLength, const int num_masks = 0) {
    Dataset d;
    d.setData("a", makeData<double>({Dim::X, axisLength}));
    for (int i = 0; i < num_masks; ++i) {
      auto bools = make_bools(axisLength, {false, true});
      d.setMask(std::string(1, ('a' + i)),
                makeVariable<bool>(Dims{Dim::X}, Shape{axisLength},
                                   Values(bools.begin(), bools.end())));
    }
    return d;
  }
};

struct Generate_2D_data {
  Dataset operator()(const int axisLength, const int num_masks = 0) {
    Dataset d;
    d.setData("a",
              makeData<double>({{Dim::X, axisLength}, {Dim::Y, axisLength}}));
    auto bools = make_bools(axisLength * axisLength, {false, true});
    for (int i = 0; i < num_masks; ++i) {
      d.setMask(std::string(1, ('a' + i)),
                makeVariable<bool>(Dims{Dim::X, Dim::Y},
                                   Shape{axisLength, axisLength},
                                   Values(bools.begin(), bools.end())));
    }
    return d;
  }
};

struct Generate_3D_data {
  Dataset operator()(const int axisLength, const int num_masks = 0) {
    Dataset d;
    d.setData("a", makeData<double>({{Dim::X, axisLength},
                                     {Dim::Y, axisLength},
                                     {Dim::Z, axisLength}}));
    auto bools =
        make_bools(axisLength * axisLength * axisLength, {false, true});
    for (int i = 0; i < num_masks; ++i) {
      d.setMask(std::string(1, ('a' + i)),
                makeVariable<bool>(Dims{Dim::X, Dim::Y, Dim::Z},
                                   Shape{axisLength, axisLength, axisLength},
                                   Values(bools.begin(), bools.end())));
    }
    return d;
  }
};

template <class Gen> static void BM_Dataset_sum(benchmark::State &state) {
  const auto itemCount = state.range(0);
  const auto maskCount = state.range(1);
  const auto d = Gen()(itemCount, maskCount);
  for (auto _ : state) {
    const auto result = sum(d, Dim::X);
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
}

// -------------------------------------------------
// No masks
// -------------------------------------------------
BENCHMARK_TEMPLATE(BM_Dataset_sum, Generate)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {256, 2048},
              /* Masks count */ {0, 0}});

BENCHMARK_TEMPLATE(BM_Dataset_sum, Generate)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {2 << 12, 2 << 15},
              /* Masks count */ {0, 0}});

BENCHMARK_TEMPLATE(BM_Dataset_sum, Generate_2D_data)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {256, 2048},
              /* Masks count */ {0, 0}});

BENCHMARK_TEMPLATE(BM_Dataset_sum, Generate_3D_data)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {16, 128},
              /* Masks count */ {0, 0}});

// -------------------------------------------------
// With Masks
// -------------------------------------------------
BENCHMARK_TEMPLATE(BM_Dataset_sum, Generate)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {256, 2048},
              /* Masks count */ {1, 8}});

BENCHMARK_TEMPLATE(BM_Dataset_sum, Generate)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {2 << 12, 2 << 15},
              /* Masks count */ {1, 2}});

BENCHMARK_TEMPLATE(BM_Dataset_sum, Generate_2D_data)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {256, 2048},
              /* Masks count */ {1, 8}});

BENCHMARK_TEMPLATE(BM_Dataset_sum, Generate_3D_data)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {16, 128},
              /* Masks count */ {1, 8}});

template <class Gen> static void BM_Dataset_mean(benchmark::State &state) {
  const auto itemCount = state.range(0);
  const auto d = Gen()(itemCount);
  for (auto _ : state) {
    const auto result = mean(d, Dim::X);
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
}

// -------------------------------------------------
// No masks
// -------------------------------------------------
BENCHMARK_TEMPLATE(BM_Dataset_mean, Generate)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {256, 2048},
              /* Masks count */ {0, 0}});

BENCHMARK_TEMPLATE(BM_Dataset_mean, Generate)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {2 << 12, 2 << 15},
              /* Masks count */ {0, 0}});

BENCHMARK_TEMPLATE(BM_Dataset_mean, Generate_2D_data)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {256, 2048},
              /* Masks count */ {0, 0}});

BENCHMARK_TEMPLATE(BM_Dataset_mean, Generate_3D_data)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {16, 128},
              /* Masks count */ {0, 0}});

// -------------------------------------------------
// With Masks
// -------------------------------------------------
BENCHMARK_TEMPLATE(BM_Dataset_mean, Generate)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {256, 2048},
              /* Masks count */ {1, 8}});

BENCHMARK_TEMPLATE(BM_Dataset_mean, Generate)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {2 << 12, 2 << 15},
              /* Masks count */ {1, 2}});

BENCHMARK_TEMPLATE(BM_Dataset_mean, Generate_2D_data)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {256, 2048},
              /* Masks count */ {1, 8}});

BENCHMARK_TEMPLATE(BM_Dataset_mean, Generate_3D_data)
    ->RangeMultiplier(2)
    ->Ranges({/* Item count */ {16, 128},
              /* Masks count */ {1, 8}});

BENCHMARK_MAIN();
