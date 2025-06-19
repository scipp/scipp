// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/strides.h"

namespace scipp::core {

Strides::Strides(const std::span<const scipp::index> &strides)
    : m_strides(strides.begin(), strides.end()) {}

Strides::Strides(const std::initializer_list<scipp::index> strides)
    : m_strides(strides) {}

Strides::Strides(const Dimensions &dims) {
  scipp::index offset{1};
  resize(dims.ndim());
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

void Strides::push_back(const scipp::index i) { m_strides.push_back(i); }

void Strides::clear() { m_strides.clear(); }

void Strides::resize(const scipp::index size) { m_strides.resize(size); }

void Strides::erase(const scipp::index i) {
  m_strides.erase(m_strides.begin() + i);
}

Strides transpose(const Strides &strides, Dimensions from,
                  const std::span<const Dim> order) {
  scipp::index i = 0;
  for (const auto &dim : from)
    from.resize(dim, strides[i++]);
  from = core::transpose(from, order);
  return Strides(from.shape());
}

} // namespace scipp::core
