// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/strides.h"

namespace scipp::core {

Strides::Strides(const Dimensions &dims) {
  scipp::index offset{1};
  for (scipp::index i = dims.ndim() - 1; i >= 0; --i) {
    m_strides[i] = offset;
    offset *= dims.size(i);
  }
}

} // namespace scipp::core
