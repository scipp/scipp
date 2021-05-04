// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>

#include "scipp/core/sizes.h"
#include "scipp/core/except.h"

namespace scipp::core {

template <class Key, class Value, int16_t MaxSize>
bool small_map<Key, Value, MaxSize>::operator==(
    const small_map &other) const noexcept {
  if (size() != other.size())
    return false;
  for (const auto &key : *this)
    if (!other.contains(key) || at(key) != other.at(key))
      return false;
  return true;
}

template <class Key, class Value, int16_t MaxSize>
bool small_map<Key, Value, MaxSize>::operator!=(
    const small_map &other) const noexcept {
  return !operator==(other);
}

template <class Key, class Value, int16_t MaxSize>
typename std::array<Key, MaxSize>::const_iterator
small_map<Key, Value, MaxSize>::find(const Key &key) const {
  return std::find(begin(), end(), key);
}

template <class Key, class Value, int16_t MaxSize>
bool small_map<Key, Value, MaxSize>::empty() const noexcept {
  return m_size == 0;
}

template <class Key, class Value, int16_t MaxSize>
bool small_map<Key, Value, MaxSize>::contains(const Key &key) const noexcept {
  return std::find(begin(), end(), key) != end();
}

template <class Key, class Value, int16_t MaxSize>
const Value &small_map<Key, Value, MaxSize>::operator[](const Key &key) const {
  return at(key);
}

template <class Key, class Value, int16_t MaxSize>
Value &small_map<Key, Value, MaxSize>::operator[](const Key &key) {
  return at(key);
}

template <class Key, class Value, int16_t MaxSize>
const Value &small_map<Key, Value, MaxSize>::at(const Key &key) const {
  if (!contains(key))
    throw std::runtime_error("Key not found");
  return m_values[std::distance(begin(), find(key))];
}

template <class Key, class Value, int16_t MaxSize>
Value &small_map<Key, Value, MaxSize>::at(const Key &key) {
  if (!contains(key))
    throw std::runtime_error("Key not found");
  return m_values[std::distance(begin(), find(key))];
}

template <class Key, class Value, int16_t MaxSize>
void small_map<Key, Value, MaxSize>::insert(const Key &key,
                                            const Value &value) {
  // TODO better to throw here?
  if (contains(key)) {
    at(key) = value;
    return;
  }
  if (size() == MaxSize)
    throw std::runtime_error("Exceeding builtin map size");
  m_keys[m_size] = key;
  m_values[m_size] = value;
  m_size++;
}

template <class Key, class Value, int16_t MaxSize>
void small_map<Key, Value, MaxSize>::erase(const Key &key) {
  if (!contains(key))
    throw std::runtime_error("Key not found");
  m_size--;
  for (scipp::index i = std::distance(begin(), find(key)); i < size(); ++i) {
    m_keys[i] = m_keys[i + 1];
    m_values[i] = m_values[i + 1];
  }
}

template <class Key, class Value, int16_t MaxSize>
void small_map<Key, Value, MaxSize>::clear() {
  m_size = 0;
}

template class small_map<Dim, scipp::index, NDIM_MAX>;

Sizes::Sizes(const Dimensions &dims) {
  for (const auto dim : dims.labels())
    insert(dim, dims[dim]);
}

Sizes::Sizes(const std::unordered_map<Dim, scipp::index> &sizes) {
  for (const auto &[dim, size] : sizes)
    insert(dim, size);
}

scipp::index &Sizes::at(const Dim dim) {
  scipp::expect::contains(*this, dim);
  return m_sizes.at(dim);
}

void Sizes::set(const Dim dim, const scipp::index size) {
  if (contains(dim) && operator[](dim) != size)
    throw except::DimensionError(
        "Inconsistent size for dim '" + to_string(dim) + "', given " +
        std::to_string(at(dim)) + ", requested " + std::to_string(size));
  insert(dim, size);
}

void Sizes::relabel(const Dim from, const Dim to) {
  if (!contains(from))
    return;
  auto size = at(from);
  erase(from);
  insert(to, size);
}

bool Sizes::contains(const Dimensions &dims) const {
  for (const auto &dim : dims.labels())
    if (!contains(dim) || at(dim) != dims[dim])
      return false;
  return true;
}

bool Sizes::contains(const Sizes &sizes) const {
  for (const auto &dim : sizes)
    if (!contains(dim) || at(dim) != sizes[dim])
      return false;
  return true;
}

Sizes Sizes::slice(const Slice &params) const {
  core::expect::validSlice(*this, params);
  Sizes sliced(*this);
  if (params.isRange())
    sliced.at(params.dim()) = params.end() - params.begin();
  else
    sliced.erase(params.dim());
  return sliced;
}

Sizes concatenate(const Sizes &a, const Sizes &b, const Dim dim) {
  Sizes out = a.contains(dim) ? a.slice({dim, 0}) : a;
  out.set(dim, (a.contains(dim) ? a[dim] : 1) + (b.contains(dim) ? b[dim] : 1));
  return out;
}

Sizes merge(const Sizes &a, const Sizes &b) {
  auto out(a);
  for (const auto &dim : b)
    out.set(dim, b[dim]);
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
  for (const auto &dim : sizes)
    repr += to_string(dim) + ":" + std::to_string(sizes[dim]) + ", ";
  repr += "]";
  return repr;
}

} // namespace scipp::core
