// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file

#include "scipp/core/dtype.h"
#include "scipp/core/except.h"

namespace scipp::core {
bool isInt(DType tp) { return tp == dtype<int32_t> || tp == dtype<int64_t>; }

DType event_dtype(const DType type) {
  switch (type) {
  case dtype<event_list<double>>:
    return dtype<double>;
  case dtype<event_list<float>>:
    return dtype<float>;
  case dtype<event_list<int64_t>>:
    return dtype<int64_t>;
  case dtype<event_list<int32_t>>:
    return dtype<int32_t>;
  default:
    return type; // event data with scalar weights
  }
}
} // namespace scipp::core
