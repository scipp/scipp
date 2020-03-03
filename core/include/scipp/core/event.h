// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_EVENT_H
#define SCIPP_CORE_EVENT_H

#include "scipp/core/dtype.h"

namespace scipp::core {

/// Return true if a variable or data array contains events
template <class T> bool is_events(const T &data) {
  const auto type = data.dtype();
  return type == dtype<sparse_container<double>> ||
         type == dtype<sparse_container<float>> ||
         type == dtype<sparse_container<int64_t>> ||
         type == dtype<sparse_container<int32_t>>;
}

} // namespace scipp::core

#endif // SCIPP_CORE_EVENT_H
