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

namespace scipp::core::dict_detail {
template <class It1, class It2, class F> struct ValueType {
  using type = std::add_rvalue_reference_t<
      std::invoke_result_t<F, typename ValueType<It1, It2, void>::type>>;
};

template <class It1, class It2> struct ValueType<It1, It2, void> {
  using type = std::add_rvalue_reference_t<
      std::pair<const typename It1::value_type,
                std::add_lvalue_reference_t<typename It2::value_type>>>;
};

template <class It1> struct ValueType<It1, void, void> {
  using type = std::add_lvalue_reference_t<typename It1::value_type>;
};

template <class F> class TransformHolder {
public:
  template <class T>
  explicit TransformHolder(T &&func) : m_func(std::forward<T>(func)) {}

  template <class T> decltype(auto) call_transform(T &&x) const {
    return m_func(std::forward<T>(x));
  }

private:
  mutable std::decay_t<F> m_func;
};

template <> class TransformHolder<void> {};

template <class Container, class It1, class It2, class F,
          size_t... IteratorIndices>
class Iterator : private TransformHolder<F> {
  static_assert(sizeof...(IteratorIndices) > 0 &&
                sizeof...(IteratorIndices) < 3);

public:
  using difference_type = std::ptrdiff_t;
  using value_type = typename ValueType<It1, It2, F>::type;
  using pointer = std::add_pointer_t<std::remove_reference_t<value_type>>;
  using reference = std::add_lvalue_reference_t<value_type>;

  static constexpr bool has_transform = !std::is_same_v<F, void>;

  template <class T>
  Iterator(std::reference_wrapper<Container> container, T &&it1)
      : m_iterators{std::forward<T>(it1)}, m_container(container),
        m_end_address(container.get().data() + container.get().size()) {}

  template <class T, class U>
  Iterator(std::reference_wrapper<Container> container, T &&it1, U &&it2)
      : m_iterators{std::forward<T>(it1), std::forward<U>(it2)},
        m_container(container),
        m_end_address(container.get().data() + container.get().size()) {}

  template <class T, class U, class S>
  Iterator(std::reference_wrapper<Container> container, T &&it1, U &&it2,
           S &&func)
      : TransformHolder<F>(std::forward<S>(func)),
        m_iterators{std::forward<T>(it1), std::forward<U>(it2)},
        m_container(container),
        m_end_address(container.get().data() + container.get().size()) {}

  Iterator(const Iterator &other) = default;
  Iterator(Iterator &&other) noexcept = default;
  Iterator &operator=(const Iterator &other) = default;
  Iterator &operator=(Iterator &&other) noexcept = default;
  ~Iterator() noexcept = default;

  decltype(auto) operator*() const {
    if constexpr (has_transform) {
      return TransformHolder<F>::call_transform(dereference());
    } else {
      return dereference();
    }
  }

  decltype(auto) operator->() const {
    if constexpr (sizeof...(IteratorIndices) == 1) {
      expect_container_unchanged();
      return std::get<0>(m_iterators);
    } else {
      return TemporaryItem(**this);
    }
  }

  Iterator &operator++() {
    expect_container_unchanged();
    (++std::get<IteratorIndices>(m_iterators), ...);
    return *this;
  }

  template <class T>
  bool operator==(const Iterator<Container, It1, It2, T, IteratorIndices...>
                      &other) const noexcept {
    expect_container_unchanged();
    // Assuming m_iterators are always in sync.
    return std::get<0>(m_iterators) == std::get<0>(other.m_iterators);
  }

  template <class T>
  bool operator!=(const Iterator<Container, It1, It2, T, IteratorIndices...>
                      &other) const noexcept {
    return !(*this == other);
  }

  friend void swap(Iterator &a, Iterator &b) {
    swap(a.m_iterators, b.m_iterators);
    swap(a.m_container, b.m_container);
    std::swap(a.m_end_address, b.m_end_address);
  }

private:
  using IteratorStorage =
      std::tuple<typename std::tuple_element<IteratorIndices,
                                             std::tuple<It1, It2>>::type...>;

  IteratorStorage m_iterators;
  std::reference_wrapper<Container> m_container;
  const void *m_end_address;

  void expect_container_unchanged() const {
    if (m_container.get().data() + m_container.get().size() != m_end_address) {
      throw std::runtime_error("dictionary changed size during iteration");
    }
  }

  decltype(auto) dereference() const {
    expect_container_unchanged();
    if constexpr (sizeof...(IteratorIndices) == 1) {
      return *std::get<0>(m_iterators);
    } else {
      return std::make_pair(std::ref(*std::get<0>(m_iterators)),
                            std::ref(*std::get<1>(m_iterators)));
    }
  }

  // operator-> needs to return a pointer or something that has operator->
  // But we cannot take the address of the temporary pair or transform result.
  // So store it in this wrapper to make it accessible via its address.
  class TemporaryItem {
  public:
    explicit TemporaryItem(value_type &&item) : m_item(std::move(item)) {}
    auto *operator->() { return &m_item; }

  private:
    std::remove_reference_t<value_type> m_item;
  };

  template <class T>
  friend class Iterator<Container, It1, It2, F, IteratorIndices...>;
};

template <class Container, class It1, class It2, class F>
Iterator(Container, It1 &&, It2 &&, F &&)
    -> Iterator<std::decay_t<Container>, std::decay_t<It1>, std::decay_t<It2>,
                std::decay_t<F>>;

template <class Container, class It1, class It2 = void, class F = void>
struct IteratorType {
  using type = Iterator<Container, It1, It2, F, 0, 1>;
};

template <class Container, class It1, class It2>
struct IteratorType<Container, It1, It2, void> {
  using type = Iterator<Container, It1, It2, void, 0, 1>;
};

template <class Container, class It1>
struct IteratorType<Container, It1, void, void> {
  using type = Iterator<Container, It1, void, void, 0>;
};
} // namespace scipp::core::dict_detail

namespace std {
template <class Container, class It1, class It2, class F,
          size_t... IteratorIndices>
struct iterator_traits<scipp::core::dict_detail::Iterator<
    Container, It1, It2, F, IteratorIndices...>> {
private:
  using I = scipp::core::dict_detail::Iterator<Container, It1, It2, F,
                                               IteratorIndices...>;

public:
  using difference_type = typename I::difference_type;
  using value_type = typename I::value_type;
  using pointer = typename I::pointer;
  using reference = typename I::reference;

  // It is a forward iterator for most use cases.
  // But it misses post-increment:
  //   it++ and *it++  (easy, but not needed right now)
  using iterator_category = std::forward_iterator_tag;
};
} // namespace std

namespace scipp::core {
template <class Key, class Value> class SCIPP_CORE_EXPORT Dict {
  using Keys = std::vector<Key>;
  using Values = std::vector<Value>;

public:
  using key_type = Key;
  using mapped_type = Value;
  using value_iterator =
      typename dict_detail::IteratorType<Values,
                                         typename Values::iterator>::type;
  using iterator =
      typename dict_detail::IteratorType<Keys, typename Keys::const_iterator,
                                         typename Values::iterator>::type;
  using const_key_iterator =
      typename dict_detail::IteratorType<const Keys,
                                         typename Keys::const_iterator>::type;
  using const_value_iterator =
      typename dict_detail::IteratorType<const Values,
                                         typename Values::const_iterator>::type;
  using const_iterator =
      typename dict_detail::IteratorType<const Keys,
                                         typename Keys::const_iterator,
                                         typename Values::const_iterator>::type;
  template <class F>
  using transform_iterator =
      typename dict_detail::IteratorType<Keys, typename Keys::const_iterator,
                                         typename Values::iterator, F>::type;

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
    return find_key(key) != m_keys.end();
  }

  template <class V> void insert_or_assign(const key_type &key, V &&value) {
    std::unique_lock lock_{m_mutex};
    if (const auto key_it = find_key(key); key_it == m_keys.end()) {
      m_keys.push_back(key);
      m_values.emplace_back(std::forward<V>(value));
    } else {
      m_values[index_of(key_it)] = std::forward<V>(value);
    }
  }

  void erase(const key_type &key) {
    std::unique_lock lock_{m_mutex};
    const auto key_it = expect_find_key(key);
    m_keys.erase(key_it);
    m_values.erase(std::next(m_values.begin(), index_of(key_it)));
  }

  mapped_type extract(const key_type &key) {
    std::unique_lock lock_{m_mutex};
    const auto key_it = expect_find_key(key);
    m_keys.erase(key_it);
    const auto value_it = std::next(m_values.begin(), index_of(key_it));
    mapped_type value = std::move(*value_it);
    m_values.erase(value_it);
    return value;
  }

  [[nodiscard]] const mapped_type &operator[](const key_type &key) const {
    std::shared_lock lock_{m_mutex};
    return m_values[expect_find_index(key)];
  }

  [[nodiscard]] mapped_type &operator[](const key_type &key) {
    std::shared_lock lock_{m_mutex};
    return m_values[expect_find_index(key)];
  }

  [[nodiscard]] const_iterator find(const key_type &key) const {
    std::shared_lock lock_{m_mutex};
    if (const auto key_it = find_key(key); key_it == m_keys.end()) {
      return end();
    } else {
      return const_iterator(m_keys, key_it,
                            std::next(m_values.begin(), index_of(key_it)));
    }
  }

  [[nodiscard]] iterator find(const key_type &key) {
    std::shared_lock lock_{m_mutex};
    if (const auto key_it = find_key(key); key_it == m_keys.end()) {
      return end();
    } else {
      return iterator(m_keys, key_it,
                      std::next(m_values.begin(), index_of(key_it)));
    }
  }

  [[nodiscard]] auto keys_begin() const noexcept {
    return const_key_iterator(m_keys, m_keys.cbegin());
  }

  [[nodiscard]] auto keys_end() const noexcept {
    return const_key_iterator(m_keys, m_keys.cend());
  }

  [[nodiscard]] auto values_begin() noexcept {
    return value_iterator(m_values, m_values.begin());
  }

  [[nodiscard]] auto values_end() noexcept {
    return value_iterator(m_values, m_values.end());
  }

  [[nodiscard]] auto values_begin() const noexcept {
    return const_value_iterator(m_values, m_values.begin());
  }

  [[nodiscard]] auto values_end() const noexcept {
    return const_value_iterator(m_values, m_values.end());
  }

  [[nodiscard]] auto begin() noexcept {
    return iterator(m_keys, m_keys.cbegin(), m_values.begin());
  }

  [[nodiscard]] auto end() noexcept {
    return iterator(m_keys, m_keys.cend(), m_values.end());
  }

  [[nodiscard]] auto begin() const noexcept {
    return const_iterator(m_keys, m_keys.cbegin(), m_values.begin());
  }

  [[nodiscard]] auto end() const noexcept {
    return const_iterator(m_keys, m_keys.cend(), m_values.begin());
  }

  template <class F> [[nodiscard]] auto begin_transform(F &&func) noexcept {
    return transform_iterator<std::decay_t<F>>(
        m_keys, m_keys.cbegin(), m_values.begin(), std::forward<F>(func));
  }

private:
  Keys m_keys;
  Values m_values;
  mutable std::shared_mutex m_mutex;

  auto find_key(const Key &key) const noexcept {
    return std::find(m_keys.begin(), m_keys.end(), key);
  }

  auto expect_find_key(const Key &key) const {
    if (const auto key_it = find_key(key); key_it != m_keys.end()) {
      return key_it;
    }
    using std::to_string;
    throw except::NotFoundError(to_string(key));
  }

  auto index_of(const typename Keys::const_iterator &it) const noexcept {
    return std::distance(m_keys.begin(), it);
  }

  scipp::index expect_find_index(const Key &key) const {
    return index_of(expect_find_key(key));
  }
};
} // namespace scipp::core
