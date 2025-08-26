// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
#include <numeric>

#include <benchmark/benchmark.h>

#include "scipp/dataset/bins.h"
#include "scipp/dataset/groupby.h"
#include "scipp/variable/astype.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

template <class T>
auto make_1d_events(const scipp::index size, const scipp::index count) {
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{size});
  scipp::index row = 0;
  for (auto &range : indices.values<std::pair<scipp::index, scipp::index>>()) {
    range = {row, row + count};
    row += count;
  }
  auto weights =
      makeVariable<T>(Dims{Dim::Event}, Shape{row}, Values{}, Variances{});
  auto y = makeVariable<T>(Dims{Dim::Event}, Shape{row});
  // TODO Check if this comment from previous implementation is still relevant:
  // Not using initializer_list to init coord map to avoid distortion of
  // benchmark --- initializer_list induces a copy and yields 2x higher
  // performance due to some details of the memory and allocation system that
  // are not entirely understood.
  DataArray buf(weights, {{Dim::Y, y}});
  // TODO Check if this comment from previous implementation is still relevant:
  // Replacing the line below by `return copy(events);` yields more than 2x
  // higher performance. It is not clear whether this is just due to improved
  // "re"-allocation performance in the benchmark loop (compared to fresh
  // allocations) or something else.
  return DataArray(make_bins(indices, Dim::Event, buf));
}

template <class T> static void BM_groupby_concat(benchmark::State &state) {
  const scipp::index nEvent = 1e8;
  const scipp::index nHist = state.range(0);
  const scipp::index nGroup = state.range(1);
  auto events = make_1d_events<T>(nHist, nEvent / nHist);
  std::vector<int64_t> group_(nHist);
  std::iota(group_.begin(), group_.end(), 0);
  auto group = makeVariable<int64_t>(Dims{Dim::X}, Shape{nHist},
                                     Values(group_.begin(), group_.end()));
  events.coords().set(
      Dim("group"),
      astype(group / (nHist / nGroup * sc_units::one), dtype<int64_t>));
  for (auto _ : state) {
    auto flat = groupby(events, Dim("group")).concat(Dim::X);
    state.PauseTiming();
    // cppcheck-suppress redundantInitialization  # Used to modify shared_ptr.
    flat = DataArray();
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * nEvent);
  // Not taking into account vector reallocations, just the raw "effective" size
  // (read event, write to output).
  int64_t data_factor = 3;
  state.SetBytesProcessed(state.iterations() * (2 * nEvent * data_factor) *
                          sizeof(T));
  state.counters["groups"] = nGroup;
  state.counters["inputs"] = nHist;
}
// Params are:
// - nHist
// - nGroup
// Also note the special case nHist = nGroup, which should effectively just make
// a copy of the input with reshuffling events.
BENCHMARK_TEMPLATE(BM_groupby_concat, float)
    ->RangeMultiplier(4)
    ->Ranges({{64, 2 << 19}, {1, 64}});
BENCHMARK_TEMPLATE(BM_groupby_concat, double)
    ->RangeMultiplier(4)
    ->Ranges({{64, 2 << 19}, {1, 64}});

static void BM_groupby_large_table(benchmark::State &state) {
  const scipp::index nCol = 3;
  const scipp::index nRow = 2 << 20;
  const scipp::index nGroup = state.range(0);
  std::vector<int64_t> group_(nRow);
  std::iota(group_.begin(), group_.end(), 0);
  Dataset d;
  const auto column = makeVariable<double>(Dims{Dim::X}, Shape{nRow});
  d.setData("a", column);
  d.setData("b", column);
  d.setData("c", column);
  d["a"].masks().set("mask", makeVariable<bool>(Dims{Dim::X}, Shape{nRow}));
  auto group = makeVariable<int64_t>(Dims{Dim::X}, Shape{nRow},
                                     Values(group_.begin(), group_.end()));
  d.coords().set(Dim("group"), astype(group / (nRow / nGroup * sc_units::one),
                                      dtype<int64_t>));
  for (auto _ : state) {
    auto grouped = groupby(d, Dim("group")).sum(Dim::X);
    state.PauseTiming();
    // cppcheck-suppress redundantInitialization  # Used to modify shared_ptr.
    // cppcheck-suppress unreadVariable
    grouped = Dataset();
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * nRow);
  state.SetBytesProcessed(state.iterations() * (nCol + 1) * (nRow + nGroup) *
                          sizeof(double));
  state.counters["groups"] = nGroup;
}

BENCHMARK(BM_groupby_large_table)->RangeMultiplier(2)->Range(64, 2 << 20);

BENCHMARK_MAIN();
