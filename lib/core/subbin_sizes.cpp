// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <numeric>

#include "scipp/common/except.h"
#include "scipp/core/element/arithmetic.h"
#include "scipp/core/element/cumulative.h"
#include "scipp/core/subbin_sizes.h"

namespace scipp::core {

SubbinSizes::SubbinSizes(const scipp::index offset, container_type &&sizes)
    : m_offset(offset), m_sizes(std::move(sizes)) {
  if (offset < 0)
    throw std::logic_error("Bad offset in class SubbinSizes.");
}

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

SubbinSizes SubbinSizes::cumsum_exclusive() const {
  auto out = sizes();
  scipp::index sum = 0;
  for (auto &x : out)
    element::exclusive_scan(sum, x);
  return {offset(), std::move(out)};
}

scipp::index SubbinSizes::sum() const {
  return std::accumulate(sizes().begin(), sizes().end(), scipp::index{0});
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

template <class Op>
SubbinSizes binary(const SubbinSizes &a, const SubbinSizes &b, Op op) {
  const auto begin = std::min(a.offset(), b.offset());
  const auto end =
      std::max(a.offset() + a.sizes().size(), b.offset() + b.sizes().size());
  typename SubbinSizes::container_type sizes(end - begin);
  scipp::index current = a.offset() - begin;
  for (const auto &size : a.sizes())
    sizes[current++] += size;
  current = b.offset() - begin;
  for (const auto &size : b.sizes())
    op(sizes[current++], size);
  return {begin, std::move(sizes)};
}

SubbinSizes operator+(const SubbinSizes &a, const SubbinSizes &b) {
  return binary(a, b, element::add_equals);
}

SubbinSizes operator-(const SubbinSizes &a, const SubbinSizes &b) {
  return binary(a, b, element::subtract_equals);
}

/** Perform a step of an exclusive scan.
 *
 * The instance is the accumulant, the argument is the next value to be added.
 * Trims the accumulant to the offset and size of the argument.
 *
 * Note that effective cache use matters here, so we do not implemented this
 * via suboperations but a single loop.
 */
void SubbinSizes::exclusive_scan(SubbinSizes &x) {
  const scipp::index osize = scipp::size(sizes());
  const scipp::index size = scipp::size(x.sizes());
  const auto delta = x.offset() - offset();
  m_sizes.reserve(size);
  m_offset = x.m_offset;
  for (scipp::index i = 0; i < size; ++i) {
    const auto prev = delta + i >= osize ? 0 : m_sizes[delta + i];
    if (i >= osize)
      m_sizes.push_back(prev + x.m_sizes[i]);
    else
      m_sizes[i] = prev + x.m_sizes[i];
    x.m_sizes[i] = prev;
  }
  m_sizes.resize(size);
}

} // namespace scipp::core
