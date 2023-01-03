// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
  const auto n = 2ul << 26ul;
  const auto nx = state.range(0);
  const auto ny = n / nx;
  const bool outer = state.range(1);
  const bool use_variances = state.range(2);
  const auto b = makeBenchmarkVariable(Dimensions{{Dim::X, nx}, {Dim::Y, ny}},
                                       use_variances);
  auto a = copy(b.slice({outer ? Dim::X : Dim::Y, 0}));
  static constexpr auto op{[](auto &a_, const auto &b_) { a_ += b_; }};

  for ([[maybe_unused]] auto _ : state) {
    accumulate_in_place<Types>(a, b, op, "");
  }

  const scipp::index variance_factor = use_variances ? 2 : 1;
  state.SetItemsProcessed(state.iterations() * n * variance_factor);
  state.SetBytesProcessed(state.iterations() * n * variance_factor *
                          sizeof(double));
  state.counters["n_outer"] = nx;
  state.counters["n_inner"] = ny;
  state.counters["variances"] = use_variances;
  state.counters["accumulate-outer"] = outer;
  state.counters["size"] = benchmark::Counter(
      static_cast<double>(n * variance_factor * sizeof(double)),
      benchmark::Counter::kDefaults, benchmark::Counter::OneK::kIs1024);
}

BENCHMARK(BM_accumulate_in_place)
    ->RangeMultiplier(2)
    ->Ranges({{2, 2ul << 25ul}, {false, true}, {false, true}});

BENCHMARK_MAIN();
