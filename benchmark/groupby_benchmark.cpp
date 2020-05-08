// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <numeric>

#include <benchmark/benchmark.h>

#include "scipp/dataset/groupby.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

template <class T>
auto make_1d_events_scalar_weights(const scipp::index size,
                                   const scipp::index count) {
  Variable var = makeVariable<event_list<T>>(Dims{Dim::X}, Shape{size});
  auto vals = var.values<event_list<T>>();
  for (scipp::index i = 0; i < size; ++i)
    vals[i].resize(count);
  // Not using initializer_list to init coord map to avoid distortion of
  // benchmark --- initializer_list induces a copy and yields 2x higher
  // performance due to some details of the memory and allocation system that
  // are not entirely understood.
  std::map<Dim, Variable> map;
  map.emplace(Dim::Y, std::move(var));
  DataArray events(makeVariable<double>(Dims{Dim::X}, Shape{size},
                                        units::counts, Values{}, Variances{}),
                   std::move(map));
  return events;
}

template <class T>
auto make_1d_events(const scipp::index size, const scipp::index count) {
  Variable var = makeVariable<event_list<T>>(Dims{Dim::X}, Shape{size},
                                             Values{}, Variances{});
  auto vals = var.values<event_list<T>>();
  auto vars = var.variances<event_list<T>>();
  for (scipp::index i = 0; i < size; ++i) {
    vals[i].resize(count);
    vars[i].resize(count);
  }
  auto events = make_1d_events_scalar_weights<T>(size, count);
  events.setData(std::move(var));
  // Replacing the line below by `return copy(events);` yields more than 2x
  // higher performance. It is not clear whether this is just due to improved
  // "re"-allocation performance in the benchmark loop (compared to fresh
  // allocations) or something else.
  return events;
}

template <class T> static void BM_groupby_flatten(benchmark::State &state) {
  const scipp::index nEvent = 1e8;
  const scipp::index nHist = state.range(0);
  const scipp::index nGroup = state.range(1);
  const bool coord_only = state.range(2);
  auto events = coord_only
                    ? make_1d_events_scalar_weights<T>(nHist, nEvent / nHist)
                    : make_1d_events<T>(nHist, nEvent / nHist);
  std::vector<int64_t> group_(nHist);
  std::iota(group_.begin(), group_.end(), 0);
  auto group = makeVariable<int64_t>(Dims{Dim::X}, Shape{nHist},
                                     Values(group_.begin(), group_.end()));
  events.coords().set(Dim("group"), group / (nHist / nGroup * units::one));
  for (auto _ : state) {
    auto flat = groupby(events, Dim("group")).flatten(Dim::X);
    state.PauseTiming();
    flat = DataArray();
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * nEvent);
  // Not taking into account vector reallocations, just the raw "effective" size
  // (read event, write to output).
  int64_t data_factor = coord_only ? 1 : 3;
  state.SetBytesProcessed(state.iterations() * (2 * nEvent * data_factor) *
                          sizeof(T));
  state.counters["coord-only"] = coord_only;
  state.counters["groups"] = nGroup;
  state.counters["inputs"] = nHist;
}
// Params are:
// - nHist
// - nGroup
// - coord_only
// Also note the special case nHist = nGroup, which should effectively just make
// a copy of the input with reshuffling events.
BENCHMARK_TEMPLATE(BM_groupby_flatten, float)
    ->RangeMultiplier(4)
    ->Ranges({{64, 2 << 19}, {1, 64}, {false, true}});
BENCHMARK_TEMPLATE(BM_groupby_flatten, double)
    ->RangeMultiplier(4)
    ->Ranges({{64, 2 << 19}, {1, 64}, {false, true}});

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
  auto group = makeVariable<int64_t>(Dims{Dim::X}, Shape{nRow},
                                     Values(group_.begin(), group_.end()));
  d.coords().set(Dim("group"), group / (nRow / nGroup * units::one));
  for (auto _ : state) {
    auto grouped = groupby(d, Dim("group")).sum(Dim::X);
    state.PauseTiming();
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
