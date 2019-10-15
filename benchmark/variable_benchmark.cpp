// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <benchmark/benchmark.h>

#include "common.h"

#include "scipp/core/variable.h"

using namespace scipp::core;
using namespace scipp;

template <typename T> struct Generate1D {
  auto operator()(int length) {
    return std::make_tuple(makeVariable<T>({Dim::X, length}),
                           sizeof(T) * length);
  }
};

template <typename T> struct Generate2D {
  auto operator()(int length) {
    return std::make_tuple(
        makeVariable<T>({{Dim::X, length}, {Dim::Y, length}}),
        sizeof(T) * std::pow(length, 2));
  }
};

template <typename T> struct Generate3D {
  auto operator()(int length) {
    return std::make_tuple(
        makeVariable<T>({{Dim::X, length}, {Dim::Y, length}, {Dim::Z, length}}),
        sizeof(T) * std::pow(length, 3));
  }
};

template <typename T> struct Generate4D {
  auto operator()(int length) {
    return std::make_tuple(makeVariable<T>({{Dim::X, length},
                                            {Dim::Y, length},
                                            {Dim::Z, length},
                                            {Dim::Qx, length}}),
                           sizeof(T) * std::pow(length, 4));
  }
};

template <typename T> struct Generate5D {
  auto operator()(int length) {
    return std::make_tuple(makeVariable<T>({{Dim::X, length},
                                            {Dim::Y, length},
                                            {Dim::Z, length},
                                            {Dim::Qx, length},
                                            {Dim::Qy, length}}),
                           sizeof(T) * std::pow(length, 5));
  }
};

template <typename T> struct Generate6D {
  auto operator()(int length) {
    return std::make_tuple(makeVariable<T>({{Dim::X, length},
                                            {Dim::Y, length},
                                            {Dim::Z, length},
                                            {Dim::Qx, length},
                                            {Dim::Qy, length},
                                            {Dim::Qz, length}}),
                           sizeof(T) * std::pow(length, 6));
  }
};

template <class Gen> static void BM_Variable_copy(benchmark::State &state) {
  const auto axisLength = state.range(0);
  auto [var, size] = Gen()(axisLength);
  for (auto _ : state) {
    Variable copy(var);
  }
  state.counters["SizeBytes"] = size;
  state.counters["BandwidthBytes"] =
      benchmark::Counter(size, benchmark::Counter::kIsIterationInvariantRate,
                         benchmark::Counter::OneK::kIs1024);
}
static void Args_Variable_copy(benchmark::internal::Benchmark *b) {
  b->Arg(10)->Arg(20);
}
static void Args_Variable_copy_sparse(benchmark::internal::Benchmark *b) {
  b->Range(1 << 5, 1 << 12);
}
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate1D<float>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate2D<float>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate3D<float>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate4D<float>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate5D<float>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate6D<float>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate1D<double>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate2D<double>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate3D<double>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate4D<double>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate5D<double>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, Generate6D<double>)
    ->Apply(Args_Variable_copy);
BENCHMARK_TEMPLATE(BM_Variable_copy, GenerateSparse<float>)
    ->Apply(Args_Variable_copy_sparse);
BENCHMARK_TEMPLATE(BM_Variable_copy, GenerateSparse<double>)
    ->Apply(Args_Variable_copy_sparse);

static void BM_Variable_trivial_slice(benchmark::State &state) {
  auto var = makeVariable<double>({{Dim::Z, 10}, {Dim::Y, 20}, {Dim::X, 30}});

  for (auto _ : state) {
    VariableProxy view(var);
    Variable copy(view);
  }
}
BENCHMARK(BM_Variable_trivial_slice);

// The following two benchmarks "prove" that operator+ with a VariableProxy is
// not unintentionally converting the second argument to a temporary Variable.
static void BM_Variable_binary_with_Variable(benchmark::State &state) {
  auto var = makeVariable<double>({{Dim::Z, 10}, {Dim::Y, 20}, {Dim::X, 30}});
  Variable a(var.slice({Dim::Z, 0, 8}));

  for (auto _ : state) {
    Variable b(var.slice({Dim::Z, 1, 9}));
    auto sum = a + b;
  }
}
static void BM_Variable_binary_with_VariableProxy(benchmark::State &state) {
  auto b = makeVariable<double>({{Dim::Z, 10}, {Dim::Y, 20}, {Dim::X, 30}});
  Variable a(b.slice({Dim::Z, 0, 8}));

  for (auto _ : state) {
    auto sum = a + b.slice({Dim::Z, 1, 9});
  }
}
BENCHMARK(BM_Variable_binary_with_Variable);
BENCHMARK(BM_Variable_binary_with_VariableProxy);

BENCHMARK_MAIN();
