// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <benchmark/benchmark.h>

#include "scipp/variable/accumulate.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

using Types = std::tuple<double>;

namespace {
Variable makeBenchmarkVariable(const Dimensions &dims,
                               const bool use_variances) {
  return use_variances
             ? makeVariable<double>(Dimensions{dims}, Values{}, Variances{})
             : makeVariable<double>(Dimensions(dims));
}
} // namespace

static void BM_accumulate_in_place(benchmark::State &state) {
  const auto nx = 1000;
  const auto ny = state.range(0);
  const auto n = nx * ny;
  const bool use_variances = state.range(1);
  const bool outer = state.range(2);
  auto a = makeBenchmarkVariable(Dimensions{{Dim::X, nx}}, use_variances);
  auto b = makeBenchmarkVariable(outer ? Dimensions{{Dim::Y, ny}, {Dim::X, nx}}
                                       : Dimensions{{Dim::X, nx}, {Dim::Y, ny}},
                                 use_variances);
  static constexpr auto op{[](auto &a_, const auto &b_) { a_ += b_; }};

  for ([[maybe_unused]] auto _ : state) {
    accumulate_in_place<Types>(a, b, op);
  }

  const scipp::index variance_factor = use_variances ? 2 : 1;
  state.SetItemsProcessed(state.iterations() * n * variance_factor);
  state.SetBytesProcessed(state.iterations() * n * variance_factor *
                          sizeof(double));
  state.counters["n"] = n;
  state.counters["variances"] = use_variances;
  state.counters["accumulate-outer"] = outer;
  state.counters["size"] = benchmark::Counter(
      static_cast<double>(n * variance_factor * sizeof(double)),
      benchmark::Counter::kDefaults, benchmark::Counter::OneK::kIs1024);
}

BENCHMARK(BM_accumulate_in_place)
    ->RangeMultiplier(2)
    ->Ranges({{1, 2ul << 18ul}, {false, true}, {false, true}});

BENCHMARK_MAIN();
