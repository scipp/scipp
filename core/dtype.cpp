// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "scipp/core/dtype.h"
#include "scipp/core/except.h"

namespace scipp::core {
bool isInt(DType tp) { return tp == dtype<int32_t> || tp == dtype<int64_t>; }

DType event_dtype(const DType type) {
  if (type == dtype<event_list<double>>)
    return dtype<double>;
  if (type == dtype<event_list<float>>)
    return dtype<float>;
  if (type == dtype<event_list<int64_t>>)
    return dtype<int64_t>;
  if (type == dtype<event_list<int32_t>>)
    return dtype<int32_t>;
  return type; // event data with scalar weights
}
} // namespace scipp::core
