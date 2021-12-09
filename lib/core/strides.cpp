// SPDX-License-Identifier: BSD-3-Clause
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

Strides::Strides(const std::initializer_list<scipp::index> strides)
    : Strides{{strides.begin(), strides.end()}} {}

Strides::Strides(const Dimensions &dims) {
  scipp::index offset{1};
  for (scipp::index i = dims.ndim() - 1; i >= 0; --i) {
    m_strides[i] = offset;
    offset *= dims.size(i);
  }
}

bool Strides::operator==(const Strides &other) const noexcept {
  return m_strides == other.m_strides;
}

bool Strides::operator!=(const Strides &other) const noexcept {
  return !operator==(other);
}

void Strides::erase(const scipp::index i) {
  for (scipp::index j = i; j < scipp::size(m_strides) - 1; ++j)
    m_strides[j] = m_strides[j + 1];
  m_strides.back() = 0;
}

Strides transpose(const Strides &strides, Dimensions from,
                  const scipp::span<const Dim> order) {
  scipp::index i = 0;
  for (const auto &dim : from)
    from.resize(dim, strides[i++]);
  from = core::transpose(from, order);
  return Strides(from.shape());
}

} // namespace scipp::core
