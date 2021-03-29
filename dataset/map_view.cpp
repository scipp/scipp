// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/map_view.h"
#include "scipp/dataset/except.h"

namespace scipp::dataset {

template <class Key, class Value>
bool Dict<Key, Value>::operator==(const Dict &other) const {
  if (size() != other.size())
    return false;
  for (const auto [name, data] : *this)
    if (!other.contains(name) || data != other[name])
      return false;
  return true;
}

template <class Key, class Value>
bool Dict<Key, Value>::operator!=(const Dict &other) const {
  return !operator==(other);
}

/// Returns whether a given key is present in the view.
template <class Key, class Value>
bool Dict<Key, Value>::contains(const Key &k) const {
  return m_items.find(k) != m_items.cend();
}

/// Returns 1 or 0, depending on whether key is present in the view or not.
template <class Key, class Value>
scipp::index Dict<Key, Value>::count(const Key &k) const {
  return static_cast<scipp::index>(contains(k));
}

/// Const reference to the coordinate for given dimension.
template <class Key, class Value>
const Value &Dict<Key, Value>::operator[](const Key &key) const {
  return at(key);
}

/// Const reference to the coordinate for given dimension.
template <class Key, class Value>
const Value &Dict<Key, Value>::at(const Key &key) const {
  scipp::expect::contains(*this, key);
  return m_items.at(key);
}

/// The coordinate for given dimension.
template <class Key, class Value>
Value Dict<Key, Value>::operator[](const Key &key) {
  return std::as_const(*this).at(key);
}

/// The coordinate for given dimension.
template <class Key, class Value> Value Dict<Key, Value>::at(const Key &key) {
  return std::as_const(*this).at(key);
}

template <class Key, class Value>
void Dict<Key, Value>::set(const key_type &key, mapped_type coord) {
  // Is a good definition for things that are allowed: "would be possible to
  // concat along existing dim or extra dim"?
  if (!m_sizes.contains(coord.dims()) &&
      !is_edges(m_sizes, coord.dims(), dim_of_coord(coord, key)))
    throw std::runtime_error("cannot add coord exceeding DataArray dims");
  m_items.insert_or_assign(key, std::move(coord));
}
template <class Key, class Value>
void Dict<Key, Value>::erase(const key_type &key) {
  scipp::expect::contains(*this, key);
  m_items.erase(key);
}

template <class Key, class Value>
Value Dict<Key, Value>::extract(const key_type &key) {
  auto extracted = at(key);
  erase(key);
  return extracted;
}

template <class Key, class Value>
Dict<Key, Value> Dict<Key, Value>::slice(const Slice &params) const {
  return Dict(m_sizes.slice(params), slice_map(m_sizes, m_items, params));
}

template <class Key, class Value>
void Dict<Key, Value>::rename(const Dim from, const Dim to) {
  m_sizes.relabel(from, to);
  // TODO relabel only if coords (not attrs?)?
  if constexpr (std::is_same_v<Key, Dim>) {
    if (m_items.count(from)) {
      auto node = m_items.extract(from);
      node.key() = to;
      m_items.insert(std::move(node));
    }
  }
  for (auto &item : m_items)
    item.second.rename(from, to);
}

template class SCIPP_DATASET_EXPORT Dict<Dim, Variable>;
template class SCIPP_DATASET_EXPORT Dict<std::string, Variable>;

} // namespace scipp::dataset
