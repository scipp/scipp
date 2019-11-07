// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_DATASET_UTIL_H
#define SCIPP_CORE_DATASET_UTIL_H

#include "scipp/core/dataset.h"

namespace scipp::core {

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
} // namespace scipp::core

#endif // SCIPP_CORE_DATASET_UTIL_H
