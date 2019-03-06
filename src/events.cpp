/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <algorithm>

#include "dataset.h"
#include "events.h"
#include "except.h"

namespace events {

void sortByTof(Dataset &dataset) {
  for (const auto &var : dataset) {
    if (var.tag() == Data::Events) {
      for (auto &el : var.get(Data::Events)) {
        if (el.size() != 1)
          throw std::runtime_error(
              "Sorting for this event type is not implemented yet.");
        const auto tofs = el.get(Data::Tof);
        std::sort(tofs.begin(), tofs.end());
      }
    } else if (var.tag() == Data::EventTofs) {
      throw std::runtime_error(
          "Sorting for this event-storage mode is not implemented yet.");
    }
  }
}

} // namespace events
