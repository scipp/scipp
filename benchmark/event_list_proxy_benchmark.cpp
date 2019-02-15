/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <benchmark/benchmark.h>

#include <random>

#include "event_list_proxy.h"

static void BM_EventListProxy_push_back_baseline(benchmark::State &state) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::poisson_distribution dist(20);

  gsl::index totalCount = 0;
  for (auto _ : state) {
    Dataset d;
    gsl::index nSpec = 100000;
    d.insert(Data::EventTofs, "a", {Dim::X, nSpec});
    d.insert(Data::EventPulseTimes, "a", {Dim::X, nSpec});
    const auto tofs = d.get(Data::EventTofs, "a");
    const auto pulseTimes = d.get(Data::EventPulseTimes, "a");
    for (gsl::index i = 0; i < tofs.size(); ++i) {
      const gsl::index count = dist(mt);
      totalCount += count;
      for (int32_t i = 0; i < count; ++i) {
        tofs[i].push_back(0.0);
        pulseTimes[i].push_back(0.0);
      }
    }
  }

  state.SetItemsProcessed(totalCount);
}
BENCHMARK(BM_EventListProxy_push_back_baseline);

static void BM_EventListProxy_push_back(benchmark::State &state) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::poisson_distribution dist(20);

  gsl::index totalCount = 0;
  for (auto _ : state) {
    Dataset d;
    gsl::index nSpec = 100000;
    d.insert(Data::EventTofs, "a", {Dim::X, nSpec});
    d.insert(Data::EventPulseTimes, "a", {Dim::X, nSpec});
    EventListsProxy eventLists(d, Access::Key{Data::EventTofs, "a"},
                               Access::Key{Data::EventPulseTimes, "a"});
    for (const auto &eventList : eventLists) {
      const gsl::index count = dist(mt);
      totalCount += count;
      for (int32_t i = 0; i < count; ++i)
        eventList.push_back(0.0, 0.0);
    }
  }

  state.SetItemsProcessed(totalCount);
}
BENCHMARK(BM_EventListProxy_push_back);

static void BM_EventListProxy_read_baseline(benchmark::State &state) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::poisson_distribution dist(20);

  gsl::index totalCount = 0;
  Dataset d;
  gsl::index nSpec = 100000;
  d.insert(Data::EventTofs, "a", {Dim::X, nSpec});
  d.insert(Data::EventPulseTimes, "a", {Dim::X, nSpec});
  EventListsProxy eventLists(d, Access::Key{Data::EventTofs, "a"},
                             Access::Key{Data::EventPulseTimes, "a"});
  for (const auto &eventList : eventLists) {
    const gsl::index count = dist(mt);
    totalCount += count;
    for (int32_t i = 0; i < count; ++i)
      eventList.push_back(0.0, 0.0);
  }

  for (auto _ : state) {
    const auto tofs = d.get(Data::EventTofs, "a");
    const auto pulseTimes = d.get(Data::EventPulseTimes, "a");
    double tof = 0.0;
    double pulseTime = 0.0;
    for (gsl::index i = 0; i < tofs.size(); ++i) {
      for (size_t j = 0; j < tofs[i].size(); ++j) {
        tof += tofs[i][j];
        pulseTime += pulseTimes[i][j];
      }
    }
  }

  state.SetItemsProcessed(state.iterations() * totalCount);
}
BENCHMARK(BM_EventListProxy_read_baseline);

static void BM_EventListProxy_read(benchmark::State &state) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::poisson_distribution dist(20);

  gsl::index totalCount = 0;
  Dataset d;
  gsl::index nSpec = 100000;
  d.insert(Data::EventTofs, "a", {Dim::X, nSpec});
  d.insert(Data::EventPulseTimes, "a", {Dim::X, nSpec});
  EventListsProxy eventLists(d, Access::Key{Data::EventTofs, "a"},
                             Access::Key{Data::EventPulseTimes, "a"});
  for (const auto &eventList : eventLists) {
    const gsl::index count = dist(mt);
    totalCount += count;
    for (int32_t i = 0; i < count; ++i)
      eventList.push_back(0.0, 0.0);
  }

  for (auto _ : state) {
    EventListsProxy eventLists(d, Access::Key{Data::EventTofs, "a"},
                               Access::Key{Data::EventPulseTimes, "a"});
    double tof = 0.0;
    double pulseTime = 0.0;
    for (const auto &eventList : eventLists) {
      for (const auto &event : eventList) {
        tof += std::get<0>(event);
        pulseTime += std::get<1>(event);
      }
    }
  }

  state.SetItemsProcessed(state.iterations() * totalCount);
}
BENCHMARK(BM_EventListProxy_read);

BENCHMARK_MAIN();
