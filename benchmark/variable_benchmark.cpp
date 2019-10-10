// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <benchmark/benchmark.h>

#include "scipp/core/variable.h"

#include "../core/test/make_sparse.h"

#include <random>

using namespace scipp::core;
using namespace scipp;

template <typename T> struct Generate1D {
  Variable operator()(int size) { return makeVariable<T>({Dim::X, size}); }
};

template <typename T> struct Generate2D {
  Variable operator()(int size) {
    return makeVariable<T>({{Dim::X, size}, {Dim::Y, size}});
  }
};

template <typename T> struct Generate3D {
  Variable operator()(int size) {
    return makeVariable<T>({{Dim::X, size}, {Dim::Y, size}, {Dim::Z, size}});
  }
};

template <typename T> struct Generate4D {
  Variable operator()(int size) {
    return makeVariable<T>(
        {{Dim::X, size}, {Dim::Y, size}, {Dim::Z, size}, {Dim::Qx, size}});
  }
};

template <typename T> struct Generate5D {
  Variable operator()(int size) {
    return makeVariable<T>({{Dim::X, size},
                            {Dim::Y, size},
                            {Dim::Z, size},
                            {Dim::Qx, size},
                            {Dim::Qy, size}});
  }
};

template <typename T> struct Generate6D {
  Variable operator()(int size) {
    return makeVariable<T>({{Dim::X, size},
                            {Dim::Y, size},
                            {Dim::Z, size},
                            {Dim::Qx, size},
                            {Dim::Qy, size},
                            {Dim::Qz, size}});
  }
};

template <typename T> struct GenerateSparse {
  Variable operator()(int size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 100);

    auto a = make_sparse_variable<T>(size);

    /* Generate a random amount of sparse data for each point */
    auto vals = a.template sparseValues<T>();
    for (scipp::index i = 0; i < size; ++i)
      vals[i] = scipp::core::sparse_container<T>(dis(gen), i);

    return a;
  }
};

template <class Gen> static void BM_Variable_copy(benchmark::State &state) {
  auto var = Gen()(state.range(0));
  for (auto _ : state) {
    Variable copy(var);
  }
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

/* static void BM_Variable_trivial_slice(benchmark::State &state) { */
/*   auto var = makeVariable<double>({{Dim::Z, 10}, {Dim::Y, 20}, {Dim::X,
 * 30}}); */

/*   for (auto _ : state) { */
/*     ConstVariableSlice view(var); */
/*     Variable copy(view); */
/*   } */
/* } */
/* BENCHMARK(BM_Variable_trivial_slice); */

/* // The following two benchmarks "prove" that operator+ with a VariableSlice
 * is */
/* // not unintentionally converting the second argument to a temporary
 * Variable. */
/* static void BM_Variable_binary_with_Variable(benchmark::State &state) { */
/*   auto var = makeVariable<double>({{Dim::Z, 10}, {Dim::Y, 20}, {Dim::X,
 * 30}}); */
/*   Variable a = var(Dim::Z, 0, 8); */

/*   for (auto _ : state) { */
/*     Variable b = var(Dim::Z, 1, 9); */
/*     auto sum = a + b; */
/*   } */
/* } */
/* static void BM_Variable_binary_with_VariableSlice(benchmark::State &state) {
 */
/*   auto b = makeVariable<double>({{Dim::Z, 10}, {Dim::Y, 20}, {Dim::X, 30}});
 */
/*   Variable a = b(Dim::Z, 0, 8); */

/*   for (auto _ : state) { */
/*     auto sum = a + b(Dim::Z, 1, 9); */
/*   } */
/* } */
/* BENCHMARK(BM_Variable_binary_with_Variable); */
/* BENCHMARK(BM_Variable_binary_with_VariableSlice); */

BENCHMARK_MAIN();
