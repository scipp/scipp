// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <vector>

#include "scipp-core_export.h"
#include "scipp/core/except.h"
#include "scipp/units/dim.h"

namespace scipp::core {

namespace detail {
template <class T>
[[nodiscard]] T rename_dims(const T &obj,
                            const std::vector<std::pair<Dim, Dim>> &names,
                            const bool fail_on_unknown) {
  auto out(obj);
  for (const auto &[from, to] : names)
    if (out.contains(from))
      out.replace_key(from, to);
    else if (fail_on_unknown)
      throw except::DimensionError(
          "Cannot rename dimension " + to_string(from) +
          " since it is not contained in the input dimensions " +
          to_string(obj) + ".");
  return out;
}
} // namespace detail

} // namespace scipp::core
