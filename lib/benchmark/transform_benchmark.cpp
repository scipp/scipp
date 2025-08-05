// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <benchmark/benchmark.h>

#include <random>

#include "scipp/variable/bins.h"
#include "scipp/variable/transform.h"
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

template <bool in_place, class Func>
void run(benchmark::State &state, Func func, bool variances = false) {
  const auto nx = 100;
  const auto ny = state.range(0);
  const auto n = nx * ny;
  const Dimensions dims{{Dim::Y, ny}, {Dim::X, nx}};
  auto a = makeBenchmarkVariable(dims, variances);
  auto b = makeBenchmarkVariable(dims, variances);
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
  state.counters["n"] = n;
  state.counters["variances"] = variances;
  state.counters["size"] = benchmark::Counter(
      n * variance_factor * size_factor * sizeof(double),
      benchmark::Counter::kDefaults, benchmark::Counter::OneK::kIs1024);
}

static void BM_transform_in_place(benchmark::State &state) {
  run<true>(
      state,
      // cppcheck-suppress constParameterReference  # state must be mutable in
      // for loop.
      [](auto &state_, auto &&...args) {
        for ([[maybe_unused]] auto _ : state_) {
          transform_in_place<Types>(args..., "");
        }
      },
      state.range(1));
}

static void BM_transform_in_place_view(benchmark::State &state) {
  run<true>(
      state,
      // cppcheck-suppress constParameterReference  # state must be mutable in
      // for loop.
      [](auto &state_, const auto &a, const auto &b, auto &op) {
        Variable a_view(a);
        const Variable b_view(b);
        for ([[maybe_unused]] auto _ : state_) {
          transform_in_place<Types>(a_view, b_view, op, "");
        }
      },
      state.range(1));
}

static void BM_transform_in_place_slice(benchmark::State &state) {
  run<true>(
      state,
      // cppcheck-suppress constParameterReference  # state must be mutable in
      // for loop.
      [](auto &state_, auto &a, auto &b, auto &op) {
        // Strictly speaking our counters are off by 1% since we
        // exclude 1 out of 100 X elements here.
        auto a_slice = a.slice({Dim::X, 0, 99});
        auto b_slice = b.slice({Dim::X, 1, 100});
        for ([[maybe_unused]] auto _ : state_) {
          transform_in_place<Types>(a_slice, b_slice, op, "");
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

static void BM_transform_in_place_transposed(benchmark::State &state) {
  // small so that a row / column fits into a cacheline (hopefully)
  const auto nx = 4;
  const auto ny = state.range(0);
  const auto n = nx * ny;
  const bool use_variances = state.range(1);
  auto a = makeBenchmarkVariable(Dimensions{{Dim::Y, ny}, {Dim::X, nx}},
                                 use_variances);
  auto b = makeBenchmarkVariable(Dimensions{{Dim::X, nx}, {Dim::Y, ny}},
                                 use_variances);
  static constexpr auto op{[](auto &a_, const auto &b_) { a_ *= b_; }};

  for ([[maybe_unused]] auto _ : state) {
    transform_in_place<Types>(a, b, op, "");
  }

  const scipp::index variance_factor = use_variances ? 2 : 1;
  const scipp::index read_write_factor = 3;
  const scipp::index size_factor = 2;
  state.SetItemsProcessed(state.iterations() * n * variance_factor);
  state.SetBytesProcessed(state.iterations() * n * variance_factor *
                          read_write_factor * sizeof(double));
  state.counters["n"] = n;
  state.counters["variances"] = use_variances;
  state.counters["size"] = benchmark::Counter(
      static_cast<double>(n * variance_factor * size_factor * sizeof(double)),
      benchmark::Counter::kDefaults, benchmark::Counter::OneK::kIs1024);
}

BENCHMARK(BM_transform_in_place_transposed)
    ->RangeMultiplier(2)
    ->Ranges({{1, 2ul << 18ul}, {false, true}});

static void BM_transform(benchmark::State &state) {
  run<false>(
      state,
      [](auto &state_, auto &&...args) {
        for ([[maybe_unused]] auto _ : state_) {
          auto out = transform<Types>(args..., "");
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
      [](auto &state_, const auto &a, const auto &b, const auto &op) {
        Variable a_view(a);
        const Variable b_view(b);
        for ([[maybe_unused]] auto _ : state_) {
          auto out = transform<Types>(a_view, b_view, op, "");
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
      [](auto &state_, auto &a, auto &b, const auto &op) {
        // Strictly speaking our counters are off by 1% since we
        // exclude 1 out of 100 X elements here.
        auto a_slice = a.slice({Dim::X, 0, 99});
        auto b_slice = b.slice({Dim::X, 1, 100});
        for ([[maybe_unused]] auto _ : state_) {
          auto out = transform<Types>(a_slice, b_slice, op, "");
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
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(dims);
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<scipp::index> dist(0, 2 * nx);
  scipp::index size = 0;
  for (auto &range : indices.values<std::pair<scipp::index, scipp::index>>()) {
    const auto l = dist(mt);
    range = {size, size + l};
    size += l;
  }
  auto buf = variances ? makeVariable<double>(Dims{Dim::Event}, Shape{size},
                                              Values{}, Variances{})
                       : makeVariable<double>(Dims{Dim::Event}, Shape{size});
  auto a = make_bins(indices, Dim::Event, buf);

  // events * dense typically occurs in unit conversion
  auto b = makeVariable<double>(Dims{Dim::Y}, Shape{ny});
  static constexpr auto op{[](auto &a_, const auto &b_) { a_ *= b_; }};

  for (auto _ : state) {
    transform_in_place<Types>(a, b, op, "");
  }

  const scipp::index variance_factor = variances ? 2 : 1;
  state.SetItemsProcessed(state.iterations() * n * variance_factor);
  state.SetBytesProcessed(state.iterations() * n * variance_factor * 3 *
                          sizeof(double));
  state.counters["n"] = n;
  state.counters["variances"] = variances;
  state.counters["size"] = benchmark::Counter(
      n * variance_factor * 2 * sizeof(double), benchmark::Counter::kDefaults,
      benchmark::Counter::OneK::kIs1024);
}

BENCHMARK(BM_transform_in_place_events)
    ->RangeMultiplier(2)
    ->Ranges({{1, 2 << 18}, {8, 2 << 8}, {false, false}});

static void BM_transform_buckets_inplace_unary(benchmark::State &state) {
  const scipp::index n_bucket = 16 * 1024;
  Dimensions dims{Dim::Y, n_bucket};
  auto indices = makeVariable<std::pair<scipp::index, scipp::index>>(dims);
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<scipp::index> dist(1, 10000);
  scipp::index n_element = 0;
  auto indices_ = indices.values<std::pair<scipp::index, scipp::index>>();
  for (scipp::index n = 0; n < n_bucket; ++n) {
    scipp::index size = dist(mt);
    indices_[n] = std::pair{n_element, n_element + size};
    n_element += size;
  }
  Variable buffer = makeVariable<double>(Dims{Dim::X}, Shape{n_element});
  Variable var = make_bins(indices, Dim::X, buffer);

  for (auto _ : state) {
    transform_in_place<double>(var, [](auto &x) { x += x; }, "");
  }

  state.SetItemsProcessed(state.iterations() * n_bucket);
  state.SetBytesProcessed(state.iterations() * n_element * sizeof(double));
  state.counters["n_bucket"] =
      benchmark::Counter(n_bucket, benchmark::Counter::kDefaults,
                         benchmark::Counter::OneK::kIs1024);
  state.counters["n_element"] =
      benchmark::Counter(n_element, benchmark::Counter::kDefaults,
                         benchmark::Counter::OneK::kIs1024);
}

BENCHMARK(BM_transform_buckets_inplace_unary);

BENCHMARK_MAIN();
