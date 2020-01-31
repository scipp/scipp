// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#include <numeric>

#include <benchmark/benchmark.h>

#include "random.h"

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

auto make_2d_sparse_coord(const scipp::index size, const scipp::index count) {
  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y},
                                  Shape{size, Dimensions::Sparse});
  auto vals = var.sparseValues<double>();
  Random rand(0.0, 1000.0);
  for (scipp::index i = 0; i < size; ++i) {
    auto data = rand(count);
    vals[i].assign(data.begin(), data.end());
  }
  return var;
}

auto make_2d_sparse_coord_only(const scipp::index size,
                               const scipp::index count) {
  return DataArray(std::nullopt, {{Dim::Y, make_2d_sparse_coord(size, count)}});
}

auto make_2d_sparse(const scipp::index size, const scipp::index count) {
  auto coord = make_2d_sparse_coord(size, count);
  auto data = coord * makeVariable<double>(Values{0.0}, Variances{0.0}) + 1.0;

  return DataArray(std::move(data), {{Dim::Y, std::move(coord)}});
}

auto make_histogram(const scipp::index nEdge) {
  std::vector<double> edges_(nEdge);
  std::iota(edges_.begin(), edges_.end(), 0.0);
  auto edges = makeVariable<double>(Dims{Dim::Y}, Shape{nEdge},
                                    Values(edges_.begin(), edges_.end()));
  edges *= 1000.0 / nEdge; // ensure all events are in range

  return DataArray(makeVariable<double>(Dims{Dim::Y}, Shape{nEdge - 1},
                                        Values{}, Variances{}),
                   {{Dim::Y, std::move(edges)}});
}

// For comparison: How fast could memory for events be allocated if it were in a
// single packed array (as opposed to many small vectors).
static void BM_dense_alloc_baseline(benchmark::State &state) {
  const scipp::index total_events = state.range(0);
  for (auto _ : state) {
    std::vector<double> vals(total_events);
    std::vector<double> vars(total_events);
  }
  state.SetItemsProcessed(state.iterations() * total_events);
  int write_data = 2; // values and variances
  state.SetBytesProcessed(state.iterations() * write_data * total_events *
                          sizeof(double));
  state.counters["total_events"] = total_events;
}

BENCHMARK(BM_dense_alloc_baseline)->RangeMultiplier(4)->Ranges({{64, 2 << 20}});

static void BM_sparse_histogram_op(benchmark::State &state) {
  const scipp::index nEvent = state.range(0);
  const scipp::index nEdge = state.range(1);
  const bool inplace = state.range(2);
  const scipp::index nHist = 2e7 / nEvent;
  const bool data = state.range(3);
  const auto sparse = data ? make_2d_sparse(nHist, nEvent)
                           : make_2d_sparse_coord_only(nHist, nEvent);
  const auto histogram = make_histogram(nEdge);
  for (auto _ : state) {
    if (inplace) {
      state.PauseTiming();
      auto sparse_ = sparse;
      state.ResumeTiming();
      sparse_ *= histogram;
    } else {
      static_cast<void>(sparse * histogram);
    }
  }
  scipp::index total_events = nHist * nEvent;
  state.SetItemsProcessed(state.iterations() * total_events);
  int read_coord = 1;
  int read_data = data ? 2 : 0; // values and variances
  int write_data = 2;           // values and variances
  state.SetBytesProcessed(state.iterations() *
                          (read_coord + read_data + write_data) * total_events *
                          sizeof(double));
  state.counters["sparse-with-data"] = data;
  state.counters["total_events"] = total_events;
  state.counters["inplace"] = inplace;
}

// Params are:
// - nEvent
// - nEdge
// - inplace
// - sparse with data
BENCHMARK(BM_sparse_histogram_op)
    ->RangeMultiplier(4)
    ->Ranges({{64, 2 << 14}, {128, 2 << 11}, {true, false}, {true, false}});

BENCHMARK_MAIN();
