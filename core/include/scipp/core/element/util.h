// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/span.h"
#include "scipp/core/value_and_variance.h"

namespace scipp::core::element {

/// Set the elements referenced by a span to 0
template <class T> void zero(const scipp::span<T> &data) {
  for (auto &x : data)
    x = 0.0;
}

/// Set the elements references by the spans for values and variances to 0
template <class T> void zero(const core::ValueAndVariance<span<T>> &data) {
  zero(data.value);
  zero(data.variance);
}

} // namespace scipp::core::element

