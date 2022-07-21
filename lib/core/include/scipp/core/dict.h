// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#pragma once

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "scipp/common/index.h"

#include "scipp-core_export.h"
#include "scipp/core/except.h"

namespace scipp::core {
namespace dict_detail {
template <class Container, class It> class KeyValueIterator {
public:
  KeyValueIterator(std::reference_wrapper<Container> container, It it)
      : m_it(it), m_container(container),
        m_initial_size(container.get().size()),
        m_initial_address(container.get().data()) {}

  KeyValueIterator &operator++() {
    expect_container_unchanged();
    ++m_it;
    return *this;
  }

  auto &operator*() const {
    expect_container_unchanged();
    return *m_it;
  }

  bool operator==(const KeyValueIterator &other) const noexcept {
    return m_it == other.m_it;
  }

  bool operator!=(const KeyValueIterator &other) const noexcept {
    return !(*this == other);
  }

private:
  It m_it;
  std::reference_wrapper<Container> m_container;
  size_t m_initial_size;
  const void *m_initial_address;

  void expect_container_unchanged() const {
    if (m_container.get().size() != m_initial_size ||
        m_container.get().data() != m_initial_address) {
      throw std::runtime_error("dictionary changed size during iteration");
    }
  }
};
} // namespace dict_detail

template <class Key, class Value> class SCIPP_CORE_EXPORT Dict {
  using Keys = std::vector<Key>;
  using Values = std::vector<Value>;

public:
  using key_type = Key;
  using mapped_type = Value;
  using key_iterator =
      dict_detail::KeyValueIterator<Keys, typename Keys::iterator>;
  using value_iterator =
      dict_detail::KeyValueIterator<Values, typename Values::iterator>;
  using const_key_iterator =
      dict_detail::KeyValueIterator<const Keys, typename Keys::const_iterator>;
  using const_value_iterator =
      dict_detail::KeyValueIterator<const Values,
                                    typename Values::const_iterator>;

  // moving and destroying not thread safe
  // and only safe on LHS off assignment, not RHS
  Dict() = default;
  ~Dict() noexcept = default;
  Dict(const Dict &other)
      : m_keys(other.m_keys), m_values(other.m_values), m_mutex() {}
  Dict(Dict &&other) noexcept = default;
  Dict &operator=(const Dict &other) {
    std::unique_lock lock_self_{m_mutex};
    m_keys = other.m_keys;
    m_values = other.m_values;
    return *this;
  }
  Dict &operator=(Dict &&other) noexcept = default;

  /// Return the number of elements.
  [[nodiscard]] index size() const noexcept { return scipp::size(m_keys); }
  /// Return true if there are 0 elements.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }
  /// Return the number of elements that space is currently allocated for.
  [[nodiscard]] index capacity() const noexcept { return m_keys.capacity(); }

  void reserve(const index new_capacity) {
    std::unique_lock lock_{m_mutex};
    m_keys.reserve(new_capacity);
    m_values.reserve(new_capacity);
  }

  [[nodiscard]] bool contains(const Key &key) const noexcept {
    std::shared_lock lock_{m_mutex};
    return find(key) != -1;
  }

  // TODO return value
  template <class V> void insert_or_assign(const key_type &key, V &&value) {
    std::unique_lock lock_{m_mutex};
    if (const auto idx = find(key); idx == -1) {
      m_keys.push_back(key);
      m_values.emplace_back(std::forward<V>(value));
    } else {
      m_values[idx] = std::forward<V>(value);
    }
  }

  [[nodiscard]] const mapped_type &operator[](const key_type &key) const {
    std::shared_lock lock_{m_mutex};
    return m_values[expect_find(key)];
  }

  [[nodiscard]] mapped_type &operator[](const key_type &key) {
    std::shared_lock lock_{m_mutex};
    return m_values[expect_find(key)];
  }

  [[nodiscard]] key_iterator keys_begin() noexcept {
    return {m_keys, m_keys.begin()};
  }

  [[nodiscard]] key_iterator keys_end() noexcept {
    return {m_keys, m_keys.end()};
  }

  [[nodiscard]] const_key_iterator keys_begin() const noexcept {
    return {m_keys, m_keys.begin()};
  }

  [[nodiscard]] const_key_iterator keys_end() const noexcept {
    return {m_keys, m_keys.end()};
  }

private:
  Keys m_keys;
  Values m_values;
  mutable std::shared_mutex m_mutex;

  scipp::index find(const Key &key) const noexcept {
    const auto it = std::find(begin(m_keys), end(m_keys), key);
    if (it == end(m_keys)) {
      return -1;
    }
    return std::distance(begin(m_keys), it);
  }

  scipp::index expect_find(const Key &key) const {
    if (const auto idx = find(key); idx != -1) {
      return idx;
    }
    using std::to_string;
    throw except::NotFoundError(to_string(key));
  }
};
} // namespace scipp::core
