// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <benchmark/benchmark.h>

#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

using Types = std::tuple<std::pair<double, double>>;

template <class Func> void run(benchmark::State &state, Func func) {
  const auto nx = 100;
  const auto ny = state.range(0);
  const auto n = nx * ny;
  auto a = makeVariable<double>({{Dim::Y, ny}, {Dim::X, nx}});
  auto b = a;
  static constexpr auto op{[](auto &a_, const auto &b_) { a_ *= b_; }};

  func(state, a, b, op);

  state.SetItemsProcessed(state.iterations() * n);
  state.SetBytesProcessed(state.iterations() * n * 3 * sizeof(double));
  state.counters["size"] =
      benchmark::Counter(n * 2 * sizeof(double), benchmark::Counter::kDefaults,
                         benchmark::Counter::OneK::kIs1024);
}

static void BM_transform_in_place(benchmark::State &state) {
  run(state, [](auto &state_, auto &&... args) {
    for (auto _ : state_) {
      transform_in_place<Types>(args...);
    }
  });
}

static void BM_transform_in_place_proxy(benchmark::State &state) {
  run(state, [](auto &state_, auto &a, auto &b, auto &op) {
    VariableProxy a_proxy(a);
    VariableConstProxy b_proxy(b);
    for (auto _ : state_) {
      transform_in_place<Types>(a_proxy, b_proxy, op);
    }
  });
}

static void BM_transform_in_place_slice(benchmark::State &state) {
  run(state, [](auto &state_, auto &a, auto &b, auto &op) {
    // Strictly speaking our counters are off by 1% since we exclude 1 out of
    // 100 X elements here.
    auto a_slice = a.slice({Dim::X, 0, 99});
    auto b_slice = b.slice({Dim::X, 1, 100});
    for (auto _ : state_) {
      transform_in_place<Types>(a_slice, b_slice, op);
    }
  });
}

BENCHMARK(BM_transform_in_place)->RangeMultiplier(2)->Range(1, 2 << 18);
BENCHMARK(BM_transform_in_place_proxy)->RangeMultiplier(2)->Range(1, 2 << 18);
BENCHMARK(BM_transform_in_place_slice)->RangeMultiplier(2)->Range(1, 2 << 18);

BENCHMARK_MAIN();
