// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <numeric>

#include <benchmark/benchmark.h>

#include "scipp/dataset/sort.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

static void BM_sort_long_table(benchmark::State &state) {
  const scipp::index nRow = state.range(0);
  const scipp::index nCol = state.range(1);
  std::vector<int64_t> key_(nRow);
  int64_t k = 0;
  for (auto &value : key_) {
    value = ++k;
    if (k > 100)
      k = 0;
  }
  Dataset d;
  const auto column = makeVariable<double>(Dims{Dim::X}, Shape{nRow});
  for (scipp::index i = 0; i < nCol; ++i)
    d.setData("data_" + std::to_string(i), column);
  auto key = makeVariable<int64_t>(Dims{Dim::X}, Shape{nRow},
                                   Values(key_.begin(), key_.end()));
  for (auto _ : state) {
    static_cast<void>(sort(d, key));
  }
  state.SetItemsProcessed(state.iterations() * nRow);
  state.SetBytesProcessed(state.iterations() * nCol * nRow * sizeof(double));
  state.counters["rows"] = nRow;
}

BENCHMARK(BM_sort_long_table)
    ->RangeMultiplier(2)
    ->Ranges({{64, 2 << 20}, {1, 8}});

BENCHMARK_MAIN();
