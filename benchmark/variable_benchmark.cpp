// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <benchmark/benchmark.h>

#include "variable_common.h"

#include "scipp/variable/operations.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

template <typename T> struct Generate1D {
  auto operator()(int length) {
    return std::make_tuple(makeVariable<T>(Dims{Dim::X}, Shape{length}),
                           sizeof(T) * length);
  }
};

template <typename T> struct Generate2D {
  auto operator()(int length) {
    return std::make_tuple(
        makeVariable<T>(Dims{Dim::X, Dim::Y}, Shape{length, length}),
        sizeof(T) * std::pow(length, 2));
  }
};

template <typename T> struct Generate3D {
  auto operator()(int length) {
    return std::make_tuple(makeVariable<T>(Dims{Dim::X, Dim::Y, Dim::Z},
                                           Shape{length, length, length}),
                           sizeof(T) * std::pow(length, 3));
  }
};

template <typename T> struct Generate4D {
  auto operator()(int length) {
    return std::make_tuple(
        makeVariable<T>(Dims{Dim::X, Dim::Y, Dim::Z, Dim::Qx},
                        Shape{length, length, length, length}),
        sizeof(T) * std::pow(length, 4));
  }
};

template <typename T> struct Generate5D {
  auto operator()(int length) {
    return std::make_tuple(
        makeVariable<T>(Dims{Dim::X, Dim::Y, Dim::Z, Dim::Qx, Dim::Qy},
                        Shape{length, length, length, length, length}),
        sizeof(T) * std::pow(length, 5));
  }
};

template <typename T> struct Generate6D {
  auto operator()(int length) {
    return std::make_tuple(
        makeVariable<T>(Dims{Dim::X, Dim::Y, Dim::Z, Dim::Qx, Dim::Qy, Dim::Qz},
                        Shape{length, length, length, length, length, length}),
        sizeof(T) * std::pow(length, 6));
  }
};

template <class Gen> static void BM_Variable_copy(benchmark::State &state) {
  const auto axisLength = state.range(0);
  auto [var, size] = Gen()(axisLength);
  for (auto _ : state) {
    Variable copy(var);
    state.PauseTiming();
    copy = Variable();
    state.ResumeTiming();
  }
  constexpr auto read_write_factor = 3;
  state.SetItemsProcessed(state.iterations() * size /
                          (size / var.dims().volume()));
  state.SetBytesProcessed(state.iterations() * size * read_write_factor);
  state.counters["SizeBytes"] = size;
}
static void Args_Variable_copy(benchmark::internal::Benchmark *b) {
  b->Arg(10)->Arg(20)->Arg(30);
}
static void Args_Variable_copy_events(benchmark::internal::Benchmark *b) {
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
BENCHMARK_TEMPLATE(BM_Variable_copy, GenerateEvents<float>)
    ->Apply(Args_Variable_copy_events);
BENCHMARK_TEMPLATE(BM_Variable_copy, GenerateEvents<double>)
    ->Apply(Args_Variable_copy_events);

static void BM_Variable_trivial_slice(benchmark::State &state) {
  auto var =
      makeVariable<double>(Dims{Dim::Z, Dim::Y, Dim::X}, Shape{10, 20, 30});

  for (auto _ : state) {
    VariableView view(var);
    Variable copy(view);
  }
}
BENCHMARK(BM_Variable_trivial_slice);

// The following two benchmarks "prove" that operator+ with a VariableView is
// not unintentionally converting the second argument to a temporary Variable.
static void BM_Variable_binary_with_Variable(benchmark::State &state) {
  auto var =
      makeVariable<double>(Dims{Dim::Z, Dim::Y, Dim::X}, Shape{10, 20, 30});
  Variable a(var.slice({Dim::Z, 0, 8}));

  for (auto _ : state) {
    Variable b(var.slice({Dim::Z, 1, 9}));
    auto sum = a + b;
  }
}
static void BM_Variable_binary_with_VariableView(benchmark::State &state) {
  auto b =
      makeVariable<double>(Dims{Dim::Z, Dim::Y, Dim::X}, Shape{10, 20, 30});
  Variable a(b.slice({Dim::Z, 0, 8}));

  for (auto _ : state) {
    auto sum = a + b.slice({Dim::Z, 1, 9});
  }
}
BENCHMARK(BM_Variable_binary_with_Variable);
BENCHMARK(BM_Variable_binary_with_VariableView);

static void BM_Variable_assign_1d(benchmark::State &state) {
  const auto size = state.range(0);

  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{size});
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{size});
  for (auto _ : state) {
    b = a;
  }

  constexpr auto read_write_factor = 3;
  state.SetItemsProcessed(state.iterations() * size);
  state.SetBytesProcessed(state.iterations() * sizeof(double) * size *
                          read_write_factor);
  state.counters["SizeBytes"] = sizeof(double) * size;
}

BENCHMARK(BM_Variable_assign_1d)
    ->Unit(benchmark::kMillisecond)
    ->Arg(1e7)
    ->Arg(1e8)
    ->Arg(1e9);

static void BM_VariableView_assign_1d(benchmark::State &state) {
  const auto size = state.range(0);

  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{size});
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{size});
  VariableView bb(b);

  for (auto _ : state) {
    bb.assign(a);
  }

  constexpr auto read_write_factor = 3;
  state.SetItemsProcessed(state.iterations() * size);
  state.SetBytesProcessed(state.iterations() * sizeof(double) * size *
                          read_write_factor);
  state.counters["SizeBytes"] = sizeof(double) * size;
}

BENCHMARK(BM_VariableView_assign_1d)
    ->Unit(benchmark::kMillisecond)
    ->Arg(1e7)
    ->Arg(1e8)
    ->Arg(1e9);

static void BM_Variable_sin_rad(benchmark::State &state) {
  const auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{1000}, units::Unit{units::rad});

  for (auto _ : state) {
    benchmark::DoNotOptimize(sin(a));
  }
}
BENCHMARK(BM_Variable_sin_rad);

static void BM_Variable_sin_deg(benchmark::State &state) {
  const auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{1000}, units::Unit{units::deg});

  for (auto _ : state) {
    benchmark::DoNotOptimize(sin(a));
  }
}
BENCHMARK(BM_Variable_sin_deg);

BENCHMARK_MAIN();
