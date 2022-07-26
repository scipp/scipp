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
template <class It1, class It2> struct ValueType {
  using type = std::add_rvalue_reference_t<
      std::pair<const typename It1::value_type,
                std::add_lvalue_reference_t<typename It2::value_type>>>;
};

template <class It1> struct ValueType<It1, void> {
  using type = std::add_lvalue_reference_t<typename It1::value_type>;
};

template <class Container, class It1, class It2, size_t... IteratorIndices>
class Iterator;

template <class BaseIterator, class Func> class TransformIterator;

template <class Container, class It1, class It2, size_t... IteratorIndices>
class SCIPP_CORE_EXPORT Iterator {
  static_assert(sizeof...(IteratorIndices) > 0 &&
                sizeof...(IteratorIndices) < 3);

public:
  using difference_type = std::ptrdiff_t;
  using value_type = typename ValueType<It1, It2>::type;
  using pointer = std::add_pointer_t<std::remove_reference_t<value_type>>;
  using reference = std::add_lvalue_reference_t<value_type>;

  template <class T>
  Iterator(std::reference_wrapper<Container> container, T &&it1)
      : m_iterators{std::forward<T>(it1)}, m_container(container),
        m_end_address(container.get().data() + container.get().size()) {}

  template <class T, class U>
  Iterator(std::reference_wrapper<Container> container, T &&it1, U &&it2)
      : m_iterators{std::forward<T>(it1), std::forward<U>(it2)},
        m_container(container),
        m_end_address(container.get().data() + container.get().size()) {}

  Iterator(const Iterator &other) = default;
  Iterator(Iterator &&other) noexcept = default;
  Iterator &operator=(const Iterator &other) = default;
  Iterator &operator=(Iterator &&other) noexcept = default;
  ~Iterator() noexcept = default;

  decltype(auto) operator*() const {
    expect_container_unchanged();
    if constexpr (sizeof...(IteratorIndices) == 1) {
      return *std::get<0>(m_iterators);
    } else {
      return std::make_pair(std::ref(*std::get<0>(m_iterators)),
                            std::ref(*std::get<1>(m_iterators)));
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

  bool operator==(
      const Iterator<Container, It1, It2, IteratorIndices...> &other) const {
    expect_container_unchanged();
    // Assuming m_iterators are always in sync.
    return std::get<0>(m_iterators) == std::get<0>(other.m_iterators);
  }

  bool operator!=(
      const Iterator<Container, It1, It2, IteratorIndices...> &other) const {
    return !(*this == other);
  }

  template <class F> auto transform(F &&func) const & {
    return TransformIterator{*this, std::forward<F>(func)};
  }

  template <class F> auto transform(F &&func) && {
    return TransformIterator{std::move(*this), std::forward<F>(func)};
  }

  friend void swap(Iterator &a, Iterator &b) {
    swap(a.m_iterators, b.m_iterators);
    swap(a.m_container, b.m_container);
    std::swap(a.m_end_address, b.m_end_address);
  }

protected:
  // operator-> needs to return a pointer or something that has operator->
  // But we cannot take the address of the temporary pair or transform result.
  // So store it in this wrapper to make it accessible via its address.
  template <class T> class TemporaryItem {
  public:
    explicit TemporaryItem(T &&item) : m_item(std::move(item)) {}
    auto *operator->() { return &m_item; }

  private:
    std::decay_t<T> m_item;
  };

  template <class T> TemporaryItem(T &&) -> TemporaryItem<std::decay_t<T>>;

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
};

template <class Container, class It1, class It2 = void> struct IteratorType {
  using type = Iterator<Container, It1, It2, 0, 1>;
};

template <class Container, class It1>
struct IteratorType<Container, It1, void> {
  using type = Iterator<Container, It1, void, 0>;
};

template <class BaseIterator, class Func>
class SCIPP_CORE_EXPORT TransformIterator : public BaseIterator {
public:
  using difference_type = std::ptrdiff_t;
  using value_type =
      std::invoke_result_t<Func, typename BaseIterator::value_type>;
  using pointer = std::add_pointer_t<std::remove_reference_t<value_type>>;
  using reference = std::add_lvalue_reference_t<value_type>;

  template <class It, class F>
  TransformIterator(It &&base, F &&func)
      : BaseIterator(std::forward<It>(base)), m_func(std::forward<F>(func)) {}

  decltype(auto) operator*() const { return m_func(BaseIterator::operator*()); }

  decltype(auto) operator->() const {
    using Result = typename BaseIterator::template TemporaryItem<
        std::decay_t<decltype(**this)>>;
    return Result(**this);
  }

private:
  // mutable to support stateful function objects that change their state.
  mutable std::decay_t<Func> m_func;
};

template <class I, class F>
TransformIterator(I, F) -> TransformIterator<std::decay_t<I>, std::decay_t<F>>;
} // namespace scipp::core::dict_detail

namespace std {
template <class Container, class It1, class It2, size_t... IteratorIndices>
struct iterator_traits<scipp::core::dict_detail::Iterator<Container, It1, It2,
                                                          IteratorIndices...>> {
private:
  using I = scipp::core::dict_detail::Iterator<Container, It1, It2,
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

template <class BaseIterator, class Func>
struct iterator_traits<
    scipp::core::dict_detail::TransformIterator<BaseIterator, Func>> {
private:
  using I = scipp::core::dict_detail::TransformIterator<BaseIterator, Func>;

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
  using value_type = std::pair<const Key, Value>;
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

  Dict(std::initializer_list<std::pair<const Key, Value>> items) {
    reserve(items.size());
    for (const auto &[key, value] : items) {
      insert_or_assign(key, value);
    }
  }

  // moving and destroying not thread safe
  // and only safe on LHS of assignment, not RHS
  Dict() = default;
  ~Dict() noexcept = default;
  Dict(const Dict &other)
      : m_keys(other.m_keys), m_values(other.m_values), m_mutex() {}
  Dict(Dict &&other) noexcept
      : m_keys(std::move(other.m_keys)), m_values(std::move(other.m_values)),
        m_mutex() {}
  Dict &operator=(const Dict &other) {
    std::unique_lock lock_self_{m_mutex};
    m_keys = other.m_keys;
    m_values = other.m_values;
    return *this;
  }
  Dict &operator=(Dict &&other) noexcept {
    std::unique_lock lock_self_{m_mutex, std::defer_lock};
    try {
      lock_self_.lock();
    } catch (const std::system_error &) {
      // This allows us to make this function noexcept.
      std::terminate();
    }
    m_keys = std::move(other.m_keys);
    m_values = std::move(other.m_values);
    return *this;
  }

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

  void clear() {
    std::unique_lock lock_{m_mutex};
    m_keys.clear();
    m_values.clear();
  }

  [[nodiscard]] const mapped_type &operator[](const key_type &key) const {
    std::shared_lock lock_{m_mutex};
    return m_values[expect_find_index(key)];
  }

  [[nodiscard]] mapped_type &operator[](const key_type &key) {
    std::shared_lock lock_{m_mutex};
    return m_values[expect_find_index(key)];
  }

  [[nodiscard]] const mapped_type &at(const key_type &key) const {
    return (*this)[key];
  }

  [[nodiscard]] mapped_type &at(const key_type &key) { return (*this)[key]; }

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
    using scipp::core::to_string;
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
