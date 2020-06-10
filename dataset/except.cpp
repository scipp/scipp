// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/except.h"
#include "scipp/dataset/dataset.h"

namespace scipp::dataset::expect {

void coordsAreSuperset(const DataArrayConstView &a,
                       const DataArrayConstView &b) {
  for (const auto &[dim, coord] : b.coords())
    if (a.coords()[dim] != coord) {
      const std::string msg = "Mismatched coordinates: \"" + to_string(dim) +
                              "\"\n" + to_string(a.coords()[dim]) +
                              to_string(b.coords()[dim]);
      throw except::CoordMismatchError(msg);
    }
}
} // namespace scipp::dataset::expect
