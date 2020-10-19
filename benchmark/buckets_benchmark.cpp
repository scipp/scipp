// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <benchmark/benchmark.h>

#include "scipp/dataset/bucket.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/operations.h"

using namespace scipp;

auto make_buckets(const scipp::index size, const scipp::index count) {
  using Model = variable::DataModel<bucket<DataArray>>;
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
  return Variable{
      std::make_unique<Model>(std::move(indices), Dim::X, std::move(buffer))};
}

static void BM_buckets_concatenate(benchmark::State &state) {
  const scipp::index nBucket = state.range(0);
  const scipp::index nEvent = state.range(1);
  auto events = make_buckets(nBucket, nEvent);
  for (auto _ : state) {
    auto var = dataset::buckets::concatenate(events, events);
    state.PauseTiming();
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
    ->Ranges({{64, 1e6}, {2 << 20, 1e9}});

BENCHMARK_MAIN();
