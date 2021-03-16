// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <ostream>

#include "scipp/core/dtype.h"
#include "scipp/core/string.h"

namespace scipp::core {

bool isInt(DType tp) { return tp == dtype<int32_t> || tp == dtype<int64_t>; }

namespace {
template <class... Ts> bool is_span_impl(DType tp) {
  return (((tp == dtype<scipp::span<Ts>>) ||
           (tp == dtype<scipp::span<const Ts>>)) ||
          ...);
}
} // namespace

bool is_span(DType tp) {
  return is_span_impl<double, float, int64_t, int32_t, bool, time_point>(tp);
}

std::ostream &operator<<(std::ostream &os, const DType &dtype) {
  return os << to_string(dtype);
}

} // namespace scipp::core
