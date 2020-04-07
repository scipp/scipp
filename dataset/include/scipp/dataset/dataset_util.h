// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

/// Iteration helper for generic code supporting data array and dataset.
///
/// Example:
///     for (const auto &[name, data] : iter(dataarray_or_dataset)) {}
template <class T> static decltype(auto) iter(T &d) {
  if constexpr (std::is_same_v<T, Dataset>)
    return d;
  else
    return d.iterable_view();
}

} // namespace scipp::dataset
