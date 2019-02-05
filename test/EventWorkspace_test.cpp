/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include "range/v3/all.hpp"
#include <algorithm>

#include "test_macros.h"

#include "md_zip_view.h"

TEST(EventWorkspace, EventList) {
  Dataset e;
  e.insert(Data::Tof{}, "", {Dim::Event, 0}, 0);
  // `size()` gives number of variables, not the number of events in this case!
  // Do we need something like `count()`, returning the volume of the Dataset?
  EXPECT_EQ(e.size(), 1);
  EXPECT_EQ(e.get(Data::Tof{}).size(), 0);

  // Cannot change size of Dataset easily right now, is that a problem here? Can
  // use concatenate, but there is no `push_back` or similar:
  Dataset e2;
  e2.insert(Data::Tof{}, "", {Dim::Event, 3}, {1.1, 2.2, 3.3});
  e = concatenate(e, e2, Dim::Event);
  e = concatenate(e, e2, Dim::Event);
  EXPECT_EQ(e.get(Data::Tof{}).size(), 6);

  // Can insert pulse times if needed.
  e.insert(Data::PulseTime{}, "", e(Data::Tof{}).dimensions(),
           {2.0, 1.0, 2.1, 1.1, 3.0, 1.2});

  auto view = ranges::view::zip(e.get(Data::Tof{}), e.get(Data::PulseTime{}));
  // Sort by Tof:
  ranges::sort(
      view.begin(), view.end(),
      [](const std::pair<double, double> &a,
         const std::pair<double, double> &b) { return a.first < b.first; });

  EXPECT_TRUE(equals(e.get(Data::Tof{}), {1.1, 1.1, 2.2, 2.2, 3.3, 3.3}));
  EXPECT_TRUE(equals(e.get(Data::PulseTime{}), {2.0, 1.1, 1.0, 3.0, 2.1, 1.2}));

  // Sort by pulse time:
  ranges::sort(
      view.begin(), view.end(),
      [](const std::pair<double, double> &a,
         const std::pair<double, double> &b) { return a.second < b.second; });

  EXPECT_TRUE(equals(e.get(Data::Tof{}), {2.2, 1.1, 3.3, 1.1, 3.3, 2.2}));
  EXPECT_TRUE(equals(e.get(Data::PulseTime{}), {1.0, 1.1, 1.2, 2.0, 2.1, 3.0}));

  // Sort by pulse time then tof:
  ranges::sort(view.begin(), view.end(),
               [](const std::pair<double, double> &a,
                  const std::pair<double, double> &b) {
                 return a.second < b.second && a.first < b.first;
               });

  EXPECT_TRUE(equals(e.get(Data::Tof{}), {2.2, 1.1, 3.3, 1.1, 3.3, 2.2}));
  EXPECT_TRUE(equals(e.get(Data::PulseTime{}), {1.0, 1.1, 1.2, 2.0, 2.1, 3.0}));
}

TEST(EventWorkspace, basics) {
  Dataset d;
  d.insert(Coord::SpectrumNumber{}, {Dim::Spectrum, 3}, {1, 2, 3});

  // "X" axis (shared for all spectra).
  d.insert(Coord::Tof{}, Dimensions(Dim::Tof, 1001), 1001);

  // EventList using Dataset. There are probably better solutions so this likely
  // to change, e.g., to use a proxy object.
  Dataset e;
  e.insert(Data::Tof{}, "", {Dim::Event, 0}, 0);
  e.insert(Data::PulseTime{}, "", {Dim::Event, 0}, 0);

  // Insert empty event lists.
  d.insert(Data::Events{}, "", {Dim::Spectrum, 3}, 3, e);

  // Get event lists for all spectra.
  auto eventLists = d.get(Data::Events{});
  EXPECT_EQ(eventLists.size(), 3);

  // Modify individual event lists.
  Dataset e2;
  e2.insert(Data::Tof{}, "", {Dim::Event, 3}, {1.1, 2.2, 3.3});
  e2.insert(Data::PulseTime{}, "", {Dim::Event, 3}, 3);
  eventLists[1] = e2;
  eventLists[2] = concatenate(e2, e2, Dim::Event);

  // Insert variable for histogrammed data.
  Dimensions dims({{Dim::Tof, 1000}, {Dim::Spectrum, 3}});
  d.insert(Data::Value{}, "", dims, dims.volume());
  d.insert(Data::Variance{}, "", dims, dims.volume());

  // Make histograms.
  // Note that we could determine the correct X axis automatically, since the
  // event data type/unit imply which coordinate to use, in this case events
  // have type Data::Tof so the axis is Coord::Tof.
  auto histLabel = MDNested(MDRead(Bin<Coord::Tof>{}), MDWrite(Data::Value{}),
                            MDWrite(Data::Variance{}));
  using Histogram = decltype(histLabel)::type;
  auto view = zipMD(d, {Dim::Tof}, histLabel, MDRead(Data::Events{}));
  for (const auto &item : view) {
    const auto &hist = item.get<Histogram>();
    const auto &events = item.get<Data::Events>();
    static_cast<void>(hist);
    static_cast<void>(events);
    // TODO: Implement rebin (better name: `makeHistogram`?).
    // makeHistogram(hist, events);
  }

  // Can keep events but drop, e.g., pulse time if not needed anymore.
  for (auto &e : d.get(Data::Events{}))
    e.erase(Data::PulseTime{});

  // Can delete events fully later.
  d.erase(Data::Events{});
}

TEST(EventWorkspace, plus) {
  Dataset d;

  Dataset e;
  e.insert(Data::Tof{}, "", {Dim::Event, 10});
  e.insert(Data::PulseTime{}, "", {Dim::Event, 10});
  auto e2 = concatenate(e, e, Dim::Event);

  d.insert(Data::Events{}, "", {Dim::Spectrum, 2}, {e, e2});

  EXPECT_THROW_MSG(d - d, std::runtime_error,
                   "Subtraction of events lists not implemented.");
  EXPECT_THROW_MSG(d * d, std::runtime_error,
                   "Multiplication of events lists not implemented.");

  // Special handling: Adding datasets *concatenates* the event lists.
  auto sum = d + d;

  auto eventLists = sum.get(Data::Events{});
  EXPECT_EQ(eventLists.size(), 2);
  EXPECT_EQ(eventLists[0].get(Data::Tof{}).size(), 2 * 10);
  EXPECT_EQ(eventLists[1].get(Data::Tof{}).size(), 2 * 20);

  sum += d;

  eventLists = sum.get(Data::Events{});
  EXPECT_EQ(eventLists.size(), 2);
  EXPECT_EQ(eventLists[0].get(Data::Tof{}).size(), 3 * 10);
  EXPECT_EQ(eventLists[1].get(Data::Tof{}).size(), 3 * 20);
}
