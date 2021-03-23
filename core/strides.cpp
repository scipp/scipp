// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/strides.h"

namespace scipp::core {

Strides::Strides(const scipp::span<const scipp::index> &strides) {
  scipp::index i = 0;
  for (const auto &stride : strides)
    m_strides.at(i++) = stride;
}

Strides::Strides(const Dimensions &dims) {
  scipp::index offset{1};
  for (scipp::index i = dims.ndim() - 1; i >= 0; --i) {
    m_strides[i] = offset;
    offset *= dims.size(i);
  }
}

void Strides::erase(const scipp::index i) {
  for (scipp::index j = i; j < scipp::size(m_strides) - 1; ++j)
    m_strides[j] = m_strides[j + 1];
  m_strides.back() = 0;
}

} // namespace scipp::core
