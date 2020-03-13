// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"

namespace scipp::core {

void expectCanBroadcastFromTo(const Dimensions &source,
                              const Dimensions &target) {
  for (const auto dim : target.labels())
    if (source.contains(dim) && (source[dim] < target[dim]))
      throw except::DimensionError("Cannot broadcast/slice dimension since "
                                   "data has mismatching but smaller "
                                   "dimension extent.");
}
} // namespace scipp::core
