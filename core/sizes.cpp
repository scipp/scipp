// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/except.h"
#include "scipp/core/sizes.h"

namespace scipp::core {

Sizes::Sizes(const Dimensions &dims) {
  for (const auto dim : dims.labels())
    m_sizes[dim] = dims[dim];
}

bool Sizes::operator==(const Sizes &other) const {
  return m_sizes == other.m_sizes;
}

bool Sizes::operator!=(const Sizes &other) const { return !operator==(other); }

scipp::index Sizes::operator[](const Dim dim) const {
  if (!contains(dim))
    throw except::DimensionError("dim not found");
  return m_sizes.at(dim);
}

void Sizes::set(const Dim dim, const scipp::index size) {
  if (contains(dim) && operator[](dim) != sizes)
    throw except::DimensionError("Inconsistent size");
  m_sizes[dim] = size;
}

bool Sizes::contains(const Dimensions &dims) {
  for (const auto &dim : dims.labels())
    if (m_sizes.count(dim) == 0 || m_sizes.at(dim) != dims[dim])
      return false;
  return true;
}

Sizes Sizes::slice(const Slice &params) const {
  auto sizes = m_sizes;
  if (params.isRange())
    sizes.at(params.dim()) = params.end() - params.begin();
  else
    sizes.erase(params.dim());
  return {sizes};
}

Sizes merge(const Sizes &a, const Sizes &b) {
  auto out(a);
  for (const auto &[dim, size] : b)
    out.set(dim, size);
  return out;
}

std::string to_string(const Sizes &sizes) { return "Sizes"; }

} // namespace scipp::core
