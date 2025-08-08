// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <ranges>

#include "rename.h"
#include "scipp/core/except.h"
#include "scipp/core/sizes.h"

namespace scipp::core {

namespace {
template <class T> void expectUnique(const T &map, const Dim label) {
  if (map.contains(label))
    throw except::DimensionError("Duplicate dimension.");
}

template <class T> std::string to_string(const T &map) {
  std::string repr("[");
  for (const auto &key : map)
    repr += to_string(key) + ":" + std::to_string(map[key]) + ", ";
  repr += "]";
  return repr;
}

template <class T>
void throw_dimension_not_found_error(const T &expected, Dim actual) {
  throw except::DimensionError{"Expected dimension to be in " +
                               to_string(expected) + ", got " +
                               to_string(actual) + '.'};
}

} // namespace

template <class Key, class Value, int16_t Capacity>
bool small_stable_map<Key, Value, Capacity>::operator==(
    const small_stable_map &other) const noexcept {
  if (size() != other.size())
    return false;
  for (const auto &[key, value] : std::views::zip(m_keys, m_values)) {
    if (!other.contains(key) || other[key] != value)
      return false;
  }
  return true;
}

template <class Key, class Value, int16_t Capacity>
bool small_stable_map<Key, Value, Capacity>::operator!=(
    const small_stable_map &other) const noexcept {
  return !operator==(other);
}

template <class Key, class Value, int16_t Capacity>
typename boost::container::small_vector<Key, Capacity>::const_iterator
small_stable_map<Key, Value, Capacity>::find(const Key &key) const {
  return std::find(begin(), end(), key);
}

template <class Key, class Value, int16_t Capacity>
bool small_stable_map<Key, Value, Capacity>::contains(const Key &key) const {
  return find(key) != end();
}

template <class Key, class Value, int16_t Capacity>
scipp::index
small_stable_map<Key, Value, Capacity>::index(const Key &key) const {
  const auto it = find(key);
  if (it == end())
    throw_dimension_not_found_error(*this, key);
  return std::distance(begin(), it);
}

template <class Key, class Value, int16_t Capacity>
const Value &
small_stable_map<Key, Value, Capacity>::operator[](const Key &key) const {
  return at(key);
}

template <class Key, class Value, int16_t Capacity>
const Value &small_stable_map<Key, Value, Capacity>::at(const Key &key) const {
  return m_values[index(key)];
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::assign(const Key &key,
                                                    const Value &value) {
  m_values[index(key)] = value;
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::insert_left(const Key &key,
                                                         const Value &value) {
  expectUnique(*this, key);
  m_keys.insert(m_keys.begin(), key);
  m_values.insert(m_values.begin(), value);
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::insert_right(const Key &key,
                                                          const Value &value) {
  expectUnique(*this, key);
  m_keys.push_back(key);
  m_values.push_back(value);
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::erase(const Key &key) {
  const auto i = index(key);
  m_keys.erase(m_keys.begin() + i);
  m_values.erase(m_values.begin() + i);
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::clear() noexcept {
  m_keys.clear();
  m_values.clear();
}

template <class Key, class Value, int16_t Capacity>
void small_stable_map<Key, Value, Capacity>::replace_key(const Key &key,
                                                         const Key &new_key) {
  if (key != new_key)
    expectUnique(*this, new_key);
  m_keys[index(key)] = new_key;
}

template class small_stable_map<Dim, scipp::index, NDIM_STACK>;

void Sizes::set(const Dim dim, const scipp::index size) {
  expect::validDim(dim);
  expect::validExtent(size);
  if (contains(dim) && operator[](dim) != size)
    throw except::DimensionError(
        "Inconsistent size for dim '" + to_string(dim) + "', given " +
        std::to_string(at(dim)) + ", requested " + std::to_string(size));
  if (!contains(dim))
    insert_right(dim, size);
}

void Sizes::resize(const Dim dim, const scipp::index size) {
  expect::validExtent(size);
  assign(dim, size);
}

/// Return true if all dimensions of other contained in *this, with same size.
bool Sizes::includes(const Sizes &sizes) const {
  return std::all_of(sizes.begin(), sizes.end(), [&](const auto &dim) {
    return contains(dim) && at(dim) == sizes[dim];
  });
}

Sizes Sizes::slice(const Slice &params) const {
  core::expect::validSlice(*this, params);
  Sizes sliced(*this);
  if (params == Slice{})
    return sliced;
  if (params.isRange()) {
    const auto remainder =
        (params.end() - params.begin()) % params.stride() != 0;
    sliced.resize(params.dim(),
                  std::max(scipp::index{0},
                           (params.end() - params.begin()) / params.stride() +
                               remainder));
  } else
    sliced.erase(params.dim());
  return sliced;
}

Sizes Sizes::rename_dims(const std::vector<std::pair<Dim, Dim>> &names,
                         const bool fail_on_unknown) const {
  return detail::rename_dims(*this, names, fail_on_unknown);
}

namespace {
Sizes concat2(const Sizes &a, const Sizes &b, const Dim dim) {
  Sizes out = a.contains(dim) ? a.slice({dim, 0}) : a;
  out.set(dim, (a.contains(dim) ? a[dim] : 1) + (b.contains(dim) ? b[dim] : 1));
  return out;
}
} // namespace

Sizes concat(const std::span<const Sizes> sizes, const Dim dim) {
  auto out = sizes.front();
  for (scipp::index i = 1; i < scipp::size(sizes); ++i)
    out = concat2(out, sizes[i], dim);
  return out;
}

Sizes merge(const Sizes &a, const Sizes &b) {
  auto out(a);
  for (const auto &dim : b)
    out.set(dim, b[dim]);
  return out;
}

bool is_edges(const Sizes &sizes, const Sizes &dataSizes, const Dim dim) {
  if (dim == Dim::Invalid || !dataSizes.contains(dim))
    return false;
  // Without this if, !sizes.contains(d) below would identify 2d coords
  // with a length-2 dim as non-edge.
  if (!sizes.empty()) {
    for (const auto &d : dataSizes) {
      // !(sizes[d] == dataSizes[d]) assumes that a coordinate can only be
      // bin-edges in one dim. I.e. if its size does not match the data in `d`,
      // it cannot be a bin-edge in `dim`.
      if (d != dim && !(sizes.contains(d) && sizes[d] == dataSizes[d]))
        return false;
    }
  }
  const auto size = dataSizes[dim];
  return size == (sizes.contains(dim) ? sizes[dim] + 1 : 2);
}

} // namespace scipp::core
