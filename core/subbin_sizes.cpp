// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <numeric>

#include "scipp/core/subbin_sizes.h"

namespace scipp::core {

SubbinSizes::SubbinSizes(const scipp::index offset,
                         std::vector<scipp::index> &&sizes)
    : m_offset(offset), m_sizes(std::move(sizes)) {}

SubbinSizes::SubbinSizes(const scipp::index value)
    : m_offset(0), m_sizes({value}) {}

SubbinSizes &SubbinSizes::operator+=(const SubbinSizes &other) {
  // fprintf(stderr, "SubbinSizes+= %ld %ld %ld %ld\n", offset(),
  // other.offset(),
  //        offset() + sizes().size(), other.offset() + other.sizes().size());
  if (other.offset() < offset())
    return *this = *this + other;
  scipp::index current = other.offset() - offset();
  const auto length = current + scipp::size(other.sizes());
  if (length > scipp::size(sizes()))
    m_sizes.resize(length);
  for (const auto &x : other.sizes())
    m_sizes[current++] += x;
  return *this;
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

void SubbinSizes::trim_to(const SubbinSizes &other) {
  auto out = other;
  out = 0;
  // full index begin/end
  const auto begin = std::max(offset(), out.offset());
  const auto end = std::min(offset() + scipp::size(sizes()),
                            out.offset() + scipp::size(out.sizes()));
  const auto ibegin = sizes().begin() + std::max(begin - offset(), 0l);
  const auto obegin = out.m_sizes.begin() + std::max(begin - out.offset(), 0l);

  std::copy_n(ibegin, std::max(0l, end - begin), obegin);
  *this = out;
}

void SubbinSizes::add_intersection(const SubbinSizes &other) {
  scipp::index delta = other.offset() - offset();
  auto i = std::max(scipp::index(0), delta);
  auto j = std::max(scipp::index(0), -delta);
  for (; j < scipp::size(other.sizes()) && i < scipp::size(sizes()); ++j, ++i) {
    m_sizes[i] += other.sizes()[j];
  }
  return;
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
  if (s.sizes().size() < 10)
    for (const auto &x : s.sizes())
      out += std::to_string(x) + ", ";
  else
    out += "vector of length " + std::to_string(s.sizes().size());

  return out + "})";
}

} // namespace scipp::core
