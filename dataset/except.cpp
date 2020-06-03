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
    if (a.dims().contains(dim) && b.dims().contains(dim) &&
        a.coords()[dim] != coord)
      throw except::CoordMismatchError("Expected coords to match.");
}

} // namespace scipp::dataset::expect
