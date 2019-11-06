// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include <numeric>

#include "common.h"

#include "boost/preprocessor.hpp"
#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

enum BoolsGeneratorType { ALTERNATING, FALSE, TRUE };

template <BoolsGeneratorType type = BoolsGeneratorType::ALTERNATING>
std::vector<bool> makeBools(const scipp::index size) {
  std::vector<bool> data(size);
  for (scipp::index i = 0; i < size; ++i)
    if constexpr (type == BoolsGeneratorType::ALTERNATING) {
      data[i] = i % 2;
    } else if constexpr (type == BoolsGeneratorType::FALSE) {
      data[i] = false;
    } else {
      data[i] = true;
    }
  return data;
}

template <typename DType> Variable makeData(const Dimensions &dims) {
  std::vector<DType> data(dims.volume());
  std::iota(data.begin(), data.end(), static_cast<DType>(0));
  return makeVariable<DType>(dims, data);
}
struct Generate {
  Dataset operator()(const int axisLength, const int num_masks = 0) {
    Dataset d;
    d.setData("a", makeData<double>({Dim::X, axisLength}));
    for (int i = 0; i < num_masks; ++i) {
      d.setMask(std::string(1, ('a' + i)),
                makeVariable<bool>(
                    {Dim::X, axisLength},
                    makeBools<BoolsGeneratorType::ALTERNATING>(axisLength)));
    }
    return d;
  }
};

struct Generate_2D_data {
  Dataset operator()(const int axisLength, const int num_masks = 0) {
    Dataset d;
    d.setData("a",
              makeData<double>({{Dim::X, axisLength}, {Dim::Y, axisLength}}));
    for (int i = 0; i < num_masks; ++i) {
      d.setMask(std::string(1, ('a' + i)),
                makeVariable<bool>({{Dim::X, axisLength}, {Dim::Y, axisLength}},
                                   makeBools<BoolsGeneratorType::ALTERNATING>(
                                       axisLength * axisLength)));
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
    for (int i = 0; i < num_masks; ++i) {
      d.setMask(std::string(1, ('a' + i)),
                makeVariable<bool>({{Dim::X, axisLength},
                                    {Dim::Y, axisLength},
                                    {Dim::Z, axisLength}},
                                   makeBools<BoolsGeneratorType::ALTERNATING>(
                                       axisLength * axisLength * axisLength)));
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
