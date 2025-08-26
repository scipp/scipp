// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include "scipp/dataset/bin.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/operations.h"

#include "../test/random.h"

using namespace scipp;

auto make_buckets(const scipp::index size, const scipp::index count) {
  Dimensions dims{Dim::Y, size};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(dims);
  scipp::index current = 0;
  for (auto &range : indices.values<std::pair<scipp::index, scipp::index>>()) {
    range.first = current;
    current += count / size;
    range.second = current;
  }
  Variable data = makeVariable<double>(Dims{Dim::X}, Shape{count});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  return make_bins(std::move(indices), Dim::X, std::move(buffer));
}

static void BM_buckets_concatenate(benchmark::State &state) {
  const scipp::index nBucket = state.range(0);
  const scipp::index nEvent = state.range(1);
  auto events = make_buckets(nBucket, nEvent);
  for (auto _ : state) {
    auto var = dataset::buckets::concatenate(events, events);
    state.PauseTiming();
    // cppcheck-suppress redundantInitialization  # Used to modify shared_ptr.
    var = Variable();
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * 2 * nEvent);
  int read_and_write = 2;
  int data_and_coord = 2;
  state.SetBytesProcessed(state.iterations() * 2 * nEvent * sizeof(double) *
                          read_and_write * data_and_coord);
  state.counters["events"] = nEvent;
  state.counters["buckets"] = nBucket;
}
BENCHMARK(BM_buckets_concatenate)
    ->RangeMultiplier(4)
    ->Ranges({{64, 2ul << 19ul}, {2ul << 20ul, 2ul << 29ul}});

auto make_table(const scipp::index size) {
  Dimensions dims(Dim::Event, size);
  Variable data = makeVariable<double>(Dims{Dim::Event}, Shape{size});
  // Range is -2.0 to 2.0
  Variable x = makeRandom(dims);
  Variable y = makeRandom(dims);
  return DataArray(data, {{Dim::X, x}, {Dim::Y, y}});
}

static void BM_bucketby(benchmark::State &state) {
  const scipp::index nEvent = state.range(0);
  auto table = make_table(nEvent);
  auto edges_x =
      makeVariable<double>(Dims{Dim::X}, Shape{5}, Values{-2, -1, 0, 1, 2});
  auto edges_y =
      makeVariable<double>(Dims{Dim::Y}, Shape{5}, Values{-2, -1, 0, 1, 2});

  for (auto _ : state) {
    // cppcheck-suppress unreadVariable
    auto a = dataset::bin(table, {edges_x, edges_y});
  }
  state.SetItemsProcessed(state.iterations() * nEvent);
  state.counters["events"] = nEvent;
}
BENCHMARK(BM_bucketby)->RangeMultiplier(4)->Ranges({{64, 2ul << 23ul}});

BENCHMARK_MAIN();
