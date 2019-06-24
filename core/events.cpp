// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>

#include "range/v3/all.hpp"

#include "scipp/core/dataset.h"
#include "scipp/core/events.h"
#include "scipp/core/except.h"

namespace scipp::core {

namespace events {

void sortByTof(Dataset &dataset) {
  for (const auto & [ name, tag, var ] : dataset) {
    if (tag == Data::Events) {
      for (auto &el : var.span<Dataset>()) {
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
    } else if (tag == Data::EventTofs) {
      throw std::runtime_error(
          "Sorting for this event-storage mode is not implemented yet.");
    }
  }
}

} // namespace events
} // namespace scipp::core
