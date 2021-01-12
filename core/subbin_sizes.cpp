// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/subbin_sizes.h"

namespace scipp::core {

SubbinSizes::SubbinSizes(const scipp::index offset,
                         std::vector<scipp::index> &&sizes)
    : m_offset(offset), m_sizes(std::move(sizes)) {}

bool operator==(const SubbinSizes &a, const SubbinSizes &b) {
  return (a.offset() == b.offset()) && (a.sizes() == b.sizes());
}

// is this good enough for what we need? for cumsum we want to avoid filling in
// the area of zero padding.. instead (inclusive_scan, similar for
// exclusive_scan):
//
// sum = intersection(sum, x) # drop lower subbins
// sum += x
// x = sum
SubbinSizes operator+(const SubbinSizes &a, const SubbinSizes &b) {
  const auto begin = std::min(a.offset(), b.offset());
  const auto end =
      std::max(a.offset() + a.sizes().size(), b.offset() + b.sizes().size());
  std::vector<scipp::index> sizes(end - begin);
  scipp::index current = a.offset() - begin;
  for (const auto &size : a.sizes())
    sizes[current++] += size;
  current = b.offset() - begin;
  for (const auto &size : b.sizes())
    sizes[current++] += size;
  return {begin, std::move(sizes)};
}

} // namespace scipp::core
