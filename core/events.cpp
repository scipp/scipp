/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <algorithm>

#include "range/v3/all.hpp"

#include "dataset.h"
#include "events.h"
#include "except.h"

namespace scipp::core {

namespace events {

void sortByTof(Dataset &dataset) {
  for (const auto &var : dataset) {
    if (var.tag() == Data::Events) {
      for (auto &el : var.get(Data::Events)) {
        if (el.size() == 1) {
          const auto tofs = el.get(Data::Tof);
          std::sort(tofs.begin(), tofs.end());
        } else if (el.size() == 2) {
          auto view = ranges::view::zip(el.get(Data::Tof),
                                        el.span<int64_t>(Data::PulseTime));
          ranges::sort(
              view.begin(), view.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
        } else {
          throw std::runtime_error(
              "Sorting for this event type is not implemented yet.");
        }
      }
    } else if (var.tag() == Data::EventTofs) {
      throw std::runtime_error(
          "Sorting for this event-storage mode is not implemented yet.");
    }
  }
}

} // namespace events
} // namespace scipp::core
