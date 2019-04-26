// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <benchmark/benchmark.h>

#include "variable.h"

using namespace scipp::core;

static void BM_Variable_copy(benchmark::State &state) {
  auto var = makeVariable<double>({{Dim::Z, 10}, {Dim::Y, 20}, {Dim::X, 30}});

  for (auto _ : state) {
    Variable copy(var);
  }
}
BENCHMARK(BM_Variable_copy);

static void BM_Variable_trivial_slice(benchmark::State &state) {
  auto var = makeVariable<double>({{Dim::Z, 10}, {Dim::Y, 20}, {Dim::X, 30}});

  for (auto _ : state) {
    ConstVariableSlice view(var);
    Variable copy(view);
  }
}
BENCHMARK(BM_Variable_trivial_slice);

// The following two benchmarks "prove" that operator+ with a VariableSlice is
// not unintentionally converting the second argument to a temporary Variable.
static void BM_Variable_binary_with_Variable(benchmark::State &state) {
  auto var = makeVariable<double>({{Dim::Z, 10}, {Dim::Y, 20}, {Dim::X, 30}});
  Variable a = var(Dim::Z, 0, 8);

  for (auto _ : state) {
    Variable b = var(Dim::Z, 1, 9);
    auto sum = a + b;
  }
}
static void BM_Variable_binary_with_VariableSlice(benchmark::State &state) {
  auto b = makeVariable<double>({{Dim::Z, 10}, {Dim::Y, 20}, {Dim::X, 30}});
  Variable a = b(Dim::Z, 0, 8);

  for (auto _ : state) {
    auto sum = a + b(Dim::Z, 1, 9);
  }
}
BENCHMARK(BM_Variable_binary_with_Variable);
BENCHMARK(BM_Variable_binary_with_VariableSlice);

BENCHMARK_MAIN();
