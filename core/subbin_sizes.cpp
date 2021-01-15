// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <numeric>

#include "scipp/common/except.h"
#include "scipp/core/subbin_sizes.h"

namespace scipp::core {

SubbinSizes::SubbinSizes(const scipp::index offset, container_type &&sizes)
    : m_offset(offset), m_sizes(std::move(sizes)) {}

SubbinSizes::SubbinSizes(const scipp::index value)
    : m_offset(0), m_sizes({value}) {}

void SubbinSizes::operator=(const scipp::index value) {
  for (auto &size : m_sizes)
    size = value;
}

SubbinSizes &SubbinSizes::operator+=(const SubbinSizes &other) {
  if (other.offset() < offset()) // avoid realloc if possible
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

SubbinSizes &SubbinSizes::add_intersection(const SubbinSizes &other) {
  scipp::index delta = other.offset() - offset();
  auto i = std::max(scipp::index(0), delta);
  auto j = std::max(scipp::index(0), -delta);
  for (; j < scipp::size(other.sizes()) && i < scipp::size(sizes()); ++j, ++i) {
    m_sizes[i] += other.sizes()[j];
  }
  return *this;
}

bool operator==(const SubbinSizes &a, const SubbinSizes &b) {
  return (a.offset() == b.offset()) && (a.sizes() == b.sizes());
}

SubbinSizes operator+(const SubbinSizes &a, const SubbinSizes &b) {
  const auto begin = std::min(a.offset(), b.offset());
  const auto end =
      std::max(a.offset() + a.sizes().size(), b.offset() + b.sizes().size());
  typename SubbinSizes::container_type sizes(end - begin);
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
  typename SubbinSizes::container_type sizes(end - begin);
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
