// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/core/subbin_sizes.h"

namespace scipp::core {

SubbinSizes::SubbinSizes(const scipp::index offset,
                         std::vector<scipp::index> &&sizes)
    : m_offset(offset), m_sizes(std::move(sizes)) {}

SubbinSizes::SubbinSizes(const scipp::index value)
    : m_offset(0), m_sizes({value}) {}

SubbinSizes &SubbinSizes::operator+=(const SubbinSizes &other) {
  return *this = *this + other;
}

SubbinSizes &SubbinSizes::operator-=(const SubbinSizes &other) {
  return *this = *this - other;
}

SubbinSizes SubbinSizes::cumsum() const {
  auto out = sizes();
  std::exclusive_scan(out.begin(), out.end(), out.begin(), 0);
  return {offset(), std::move(out)};
}

scipp::index SubbinSizes::sum() const {
  return std::accumulate(sizes().begin(), sizes().end(), scipp::index{0});
}

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

SubbinSizes operator-(const SubbinSizes &a, const SubbinSizes &b) {
  const auto begin = std::min(a.offset(), b.offset());
  const auto end =
      std::max(a.offset() + a.sizes().size(), b.offset() + b.sizes().size());
  std::vector<scipp::index> sizes(end - begin);
  scipp::index current = a.offset() - begin;
  for (const auto &size : a.sizes())
    sizes[current++] += size;
  current = b.offset() - begin;
  for (const auto &size : b.sizes())
    sizes[current++] -= size;
  return {begin, std::move(sizes)};
}

std::string to_string(const SubbinSizes &s) {
  std::string out("(offset=" + std::to_string(s.offset()) + ", values={");
  for (const auto &x : s.sizes())
    out += std::to_string(x) + ", ";
  ;

  return out + "})";
}

} // namespace scipp::core
