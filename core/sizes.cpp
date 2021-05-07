// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/sizes.h"
#include "scipp/core/except.h"

namespace scipp::core {

Sizes::Sizes(const Dimensions &dims) {
  for (const auto dim : dims.labels())
    m_sizes[dim] = dims[dim];
}

bool Sizes::operator==(const Sizes &other) const {
  return m_sizes == other.m_sizes;
}

bool Sizes::operator!=(const Sizes &other) const { return !operator==(other); }

void Sizes::clear() { m_sizes.clear(); }

scipp::index Sizes::operator[](const Dim dim) const { return at(dim); }

scipp::index Sizes::at(const Dim dim) const {
  scipp::expect::contains(*this, dim);
  return m_sizes.at(dim);
}

void Sizes::set(const Dim dim, const scipp::index size) {
  if (contains(dim) && operator[](dim) != size)
    throw except::DimensionError(
        "Inconsistent size for dim '" + to_string(dim) + "', given " +
        std::to_string(at(dim)) + ", requested " + std::to_string(size));
  m_sizes[dim] = size;
}

void Sizes::erase(const Dim dim) {
  scipp::expect::contains(*this, dim);
  m_sizes.erase(dim);
}

void Sizes::relabel(const Dim from, const Dim to) {
  if (!contains(from))
    return;
  auto node = m_sizes.extract(from);
  node.key() = to;
  m_sizes.insert(std::move(node));
}

bool Sizes::contains(const Dimensions &dims) const {
  for (const auto &dim : dims.labels())
    if (m_sizes.count(dim) == 0 || m_sizes.at(dim) != dims[dim])
      return false;
  return true;
}

bool Sizes::contains(const Sizes &sizes) const {
  for (const auto &[dim, size] : sizes)
    if (m_sizes.count(dim) == 0 || m_sizes.at(dim) != size)
      return false;
  return true;
}

Sizes Sizes::slice(const Slice &params) const {
  core::expect::validSlice(*this, params);
  auto sizes = m_sizes;
  if (params.isRange())
    sizes.at(params.dim()) = params.end() - params.begin();
  else
    sizes.erase(params.dim());
  return {sizes};
}

Sizes concatenate(const Sizes &a, const Sizes &b, const Dim dim) {
  Sizes out = a.contains(dim) ? a.slice({dim, 0}) : a;
  out.set(dim, (a.contains(dim) ? a[dim] : 1) + (b.contains(dim) ? b[dim] : 1));
  return out;
}

Sizes merge(const Sizes &a, const Sizes &b) {
  auto out(a);
  for (const auto &[dim, size] : b)
    out.set(dim, size);
  return out;
}

bool is_edges(const Sizes &sizes, const Dimensions &dims, const Dim dim) {
  if (dim == Dim::Invalid)
    return false;
  for (const auto &d : dims.labels())
    if (d != dim && !(sizes.contains(d) && sizes[d] == dims[d]))
      return false;
  const auto size = dims[dim];
  return size == (sizes.contains(dim) ? sizes[dim] + 1 : 2);
}

std::string to_string(const Sizes &sizes) {
  std::string repr("Sizes[");
  for (const auto &[dim, size] : sizes)
    repr += to_string(dim) + ":" + std::to_string(size) + ", ";
  repr += "]";
  return repr;
}

} // namespace scipp::core
