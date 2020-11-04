// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <ostream>

#include "scipp/core/dtype.h"
#include "scipp/core/string.h"

namespace scipp::core {
bool isInt(DType tp) { return tp == dtype<int32_t> || tp == dtype<int64_t>; }

std::ostream &operator<<(std::ostream &os, const DType &dtype) {
  return os << to_string(dtype);
}

} // namespace scipp::core
