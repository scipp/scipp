// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#include <numeric>

#include <benchmark/benchmark.h>

#include "random.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/unaligned.h"
#include "scipp/variable/operations.h"

using namespace scipp;

auto make_2d_events_coord(const scipp::index size, const scipp::index count) {
  auto var = makeVariable<event_list<double>>(Dims{Dim::X}, Shape{size});
  auto vals = var.values<event_list<double>>();
  Random rand(0.0, 1000.0);
  for (scipp::index i = 0; i < size; ++i) {
    auto data = rand(count);
    vals[i].assign(data.begin(), data.end());
  }
  return var;
}

auto make_edges(const scipp::index nEdge) {
  std::vector<double> edges_(nEdge);
  std::iota(edges_.begin(), edges_.end(), 0.0);
  auto edges = makeVariable<double>(Dims{Dim::Y}, Shape{nEdge},
                                    Values(edges_.begin(), edges_.end()));
  edges *= 1000.0 / nEdge * units::one; // ensure all events are in range
  return edges;
}

auto make_2d_events_default_weights(const scipp::index size,
                                    const scipp::index count,
                                    const scipp::index nEdge) {
  auto weights = makeVariable<double>(Dims{Dim::X}, Shape{size}, units::counts,
                                      Values{}, Variances{});
  return dataset::unaligned::realign(
      DataArray(weights, {{Dim::Y, make_2d_events_coord(size, count)}}),
      {{Dim::Y, make_edges(nEdge)}});
}

auto make_2d_events(const scipp::index size, const scipp::index count,
                    const scipp::index nEdge) {
  auto coord = make_2d_events_coord(size, count);
  auto data = coord * makeVariable<double>(Values{0.0}, Variances{0.0}) +
              1.0 * units::one;

  return dataset::unaligned::realign(
      DataArray(std::move(data), {{Dim::Y, std::move(coord)}}),
      {{Dim::Y, make_edges(nEdge)}});
}

auto make_histogram(const scipp::index nEdge) {
  return DataArray(makeVariable<double>(Dims{Dim::Y}, Shape{nEdge - 1},
                                        Values{}, Variances{}),
                   {{Dim::Y, make_edges(nEdge)}});
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

static void BM_events_histogram_op(benchmark::State &state) {
  const scipp::index nEvent = state.range(0);
  const scipp::index nEdge = state.range(1);
  const bool inplace = state.range(2);
  const scipp::index nHist = 2e7 / nEvent;
  const bool data = state.range(3);
  const auto events =
      data ? make_2d_events(nHist, nEvent, nEdge)
           : make_2d_events_default_weights(nHist, nEvent, nEdge);
  const auto histogram = make_histogram(nEdge);
  for (auto _ : state) {
    if (inplace) {
      state.PauseTiming();
      auto events_ = events;
      state.ResumeTiming();
      events_ *= histogram;
    } else {
      static_cast<void>(events * histogram);
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
  state.counters["events-with-data"] = data;
  state.counters["total_events"] = total_events;
  state.counters["inplace"] = inplace;
}

// Params are:
// - nEvent
// - nEdge
// - inplace
// - events with data
BENCHMARK(BM_events_histogram_op)
    ->RangeMultiplier(4)
    ->Ranges({{64, 2 << 14}, {128, 2 << 11}, {true, false}, {true, false}});

BENCHMARK_MAIN();
