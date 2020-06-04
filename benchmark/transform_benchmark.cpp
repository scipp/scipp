// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <benchmark/benchmark.h>

#include <random>

#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

using Types = std::tuple<double>;

template <bool in_place, class Func>
void run(benchmark::State &state, Func func, bool variances = false) {
  const auto nx = 100;
  const auto ny = state.range(0);
  const auto n = nx * ny;
  const Dimensions dims{{Dim::Y, ny}, {Dim::X, nx}};
  auto a = variances
               ? makeVariable<double>(Dimensions{dims}, Values{}, Variances{})
               : makeVariable<double>(Dimensions(dims));
  auto b = a;
  if constexpr (in_place) {
    static constexpr auto op{[](auto &a_, const auto &b_) { a_ *= b_; }};
    func(state, a, b, op);
  } else {
    static constexpr auto op{[](auto &&a_, auto &&b_) { return a_ * b_; }};
    func(state, a, b, op);
  }

  const scipp::index variance_factor = variances ? 2 : 1;
  const scipp::index read_write_factor = in_place ? 3 : 4;
  const scipp::index size_factor = in_place ? 2 : 3;
  state.SetItemsProcessed(state.iterations() * n * variance_factor);
  state.SetBytesProcessed(state.iterations() * n * variance_factor *
                          read_write_factor * sizeof(double));
  state.counters["size"] = benchmark::Counter(
      n * variance_factor * size_factor * sizeof(double),
      benchmark::Counter::kDefaults, benchmark::Counter::OneK::kIs1024);
}

static void BM_transform_in_place(benchmark::State &state) {
  run<true>(
      state,
      [](auto &state_, auto &&... args) {
        for ([[maybe_unused]] auto _ : state_) {
          transform_in_place<Types>(args...);
        }
      },
      state.range(1));
}

static void BM_transform_in_place_view(benchmark::State &state) {
  run<true>(
      state,
      [](auto &state_, auto &a, auto &b, auto &op) {
        VariableView a_view(a);
        VariableConstView b_view(b);
        for ([[maybe_unused]] auto _ : state_) {
          transform_in_place<Types>(a_view, b_view, op);
        }
      },
      state.range(1));
}

static void BM_transform_in_place_slice(benchmark::State &state) {
  run<true>(
      state,
      [](auto &state_, auto &a, auto &b, auto &op) {
        // Strictly speaking our counters are off by 1% since we
        // exclude 1 out of 100 X elements here.
        auto a_slice = a.slice({Dim::X, 0, 99});
        auto b_slice = b.slice({Dim::X, 1, 100});
        for ([[maybe_unused]] auto _ : state_) {
          transform_in_place<Types>(a_slice, b_slice, op);
        }
      },
      state.range(1));
}

// {false, true} -> variances
BENCHMARK(BM_transform_in_place)
    ->RangeMultiplier(2)
    ->Ranges({{1, 2 << 18}, {false, true}});
BENCHMARK(BM_transform_in_place_view)
    ->RangeMultiplier(2)
    ->Ranges({{1, 2 << 18}, {false, true}});
BENCHMARK(BM_transform_in_place_slice)
    ->RangeMultiplier(2)
    ->Ranges({{1, 2 << 18}, {false, true}});

static void BM_transform(benchmark::State &state) {
  run<false>(
      state,
      [](auto &state_, auto &&... args) {
        for ([[maybe_unused]] auto _ : state_) {
          auto out = transform<Types>(args...);
          state_.PauseTiming();
          out = Variable();
          state_.ResumeTiming();
        }
      },
      state.range(1));
}

static void BM_transform_view(benchmark::State &state) {
  run<false>(
      state,
      [](auto &state_, auto &a, auto &b, auto &op) {
        VariableView a_view(a);
        VariableConstView b_view(b);
        for ([[maybe_unused]] auto _ : state_) {
          auto out = transform<Types>(a_view, b_view, op);
          state_.PauseTiming();
          out = Variable();
          state_.ResumeTiming();
        }
      },
      state.range(1));
}

static void BM_transform_slice(benchmark::State &state) {
  run<false>(
      state,
      [](auto &state_, auto &a, auto &b, auto &op) {
        // Strictly speaking our counters are off by 1% since we
        // exclude 1 out of 100 X elements here.
        auto a_slice = a.slice({Dim::X, 0, 99});
        auto b_slice = b.slice({Dim::X, 1, 100});
        for ([[maybe_unused]] auto _ : state_) {
          auto out = transform<Types>(a_slice, b_slice, op);
          state_.PauseTiming();
          out = Variable();
          state_.ResumeTiming();
        }
      },
      state.range(1));
}

// {false, true} -> variances
BENCHMARK(BM_transform)
    ->RangeMultiplier(2)
    ->Ranges({{1, 2 << 18}, {false, true}});
BENCHMARK(BM_transform_view)
    ->RangeMultiplier(2)
    ->Ranges({{1, 2 << 18}, {false, true}});
BENCHMARK(BM_transform_slice)
    ->RangeMultiplier(2)
    ->Ranges({{1, 2 << 18}, {false, true}});

// Arguments are:
// range(0) -> ny
// range(1) -> average nx (uniform distribution of events extents)
// range(2) -> variances false/true (true not implemented yet)
static void BM_transform_in_place_events(benchmark::State &state) {
  const auto ny = state.range(0);
  const auto nx = state.range(1);
  const auto n = nx * ny;
  const Dimensions dims{{Dim::Y, ny}};
  bool variances = state.range(2);
  auto a = variances ? makeVariable<event_list<double>>(Dimensions{dims},
                                                        Values{}, Variances{})
                     : makeVariable<event_list<double>>(Dimensions(dims));

  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<scipp::index> dist(0, 2 * nx);
  for (auto &elems : a.values<event_list<double>>())
    elems.resize(dist(mt));

  // events * dense typically occurs in unit conversion
  auto b = makeVariable<double>(Dims{Dim::Y}, Shape{ny});
  static constexpr auto op{[](auto &a_, const auto &b_) { a_ *= b_; }};

  for (auto _ : state) {
    transform_in_place<Types>(a, b, op);
  }

  const scipp::index variance_factor = variances ? 2 : 1;
  state.SetItemsProcessed(state.iterations() * n * variance_factor);
  state.SetBytesProcessed(state.iterations() * n * variance_factor * 3 *
                          sizeof(double));
  state.counters["size"] = benchmark::Counter(
      n * variance_factor * 2 * sizeof(double), benchmark::Counter::kDefaults,
      benchmark::Counter::OneK::kIs1024);
}

BENCHMARK(BM_transform_in_place_events)
    ->RangeMultiplier(2)
    ->Ranges({{1, 2 << 18}, {8, 2 << 8}, {false, false}});

BENCHMARK_MAIN();
