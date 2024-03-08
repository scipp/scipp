// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
///
/// Dict is a container similar to Python's dict. It differs from
/// std::map and std::unordered_map in that it stores elements in the
/// order of insertion. In addition, its iterators throw an exception
/// if the dict has changed size during iteration. This matches Python's
/// behavior and avoids segfaults when misusing the dict.
#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "scipp/common/index.h"

#include "scipp/core/except.h"
#include "scipp/core/string.h"

namespace scipp::core::dict_detail {
template <class It1, class It2 = void> struct ValueType {
  using type = std::pair<typename It1::value_type, typename It2::value_type>;
};

template <class It1> struct ValueType<It1, void> {
  using type = typename It1::value_type;
};

template <class It1, class It2 = void> struct ReferenceType {
  using type = std::add_rvalue_reference_t<
      std::pair<typename It1::reference, typename It2::reference>>;
};

template <class It1> struct ReferenceType<It1, void> {
  using type = typename It1::reference;
};

template <class BaseIterator, class Func> class TransformIterator;

// This iterator is mostly standard library conform. But it violates the
// requirement that *it must return a reference to value_type.
// This is required because the keys must be returned as const refs but
// stored in the dict as non-const.
template <class Container, class... It> class Iterator {
  static_assert(sizeof...(It) > 0 && sizeof...(It) < 3);

public:
  using difference_type = std::ptrdiff_t;
  using value_type = typename ValueType<It...>::type;
  using pointer = std::add_pointer_t<std::remove_reference_t<value_type>>;
  using reference = typename ReferenceType<It...>::type;

  template <class... T>
  explicit Iterator(std::reference_wrapper<Container> container, T &&...it)
      : m_iterators{std::forward<T>(it)...}, m_container(container),
        m_base_address(container.get().data()), m_size(container.get().size()) {
  }

  decltype(auto) operator*() const {
    expect_container_unchanged();
    if constexpr (sizeof...(It) == 1) {
      return *std::get<0>(m_iterators);
    } else {
      return std::make_pair(std::cref(*std::get<0>(m_iterators)),
                            std::ref(*std::get<1>(m_iterators)));
    }
  }

  decltype(auto) operator->() const {
    if constexpr (sizeof...(It) == 1) {
      expect_container_unchanged();
      return std::get<0>(m_iterators);
    } else {
      // No need to use expect_container_unchanged
      // because we delegate to operator*
      return TemporaryItem<reference>(**this);
    }
  }

  Iterator &operator++() {
    expect_container_unchanged();
    ++std::get<0>(m_iterators);
    if constexpr (sizeof...(It) == 2)
      ++std::get<1>(m_iterators);
    return *this;
  }

  bool operator==(const Iterator<Container, It...> &other) const {
    expect_container_unchanged();
    // Assuming m_iterators are always in sync.
    return std::get<0>(m_iterators) == std::get<0>(other.m_iterators);
  }

  bool operator!=(const Iterator<Container, It...> &other) const {
    return !(*this == other); // NOLINT
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
    std::swap(a.m_base_address, b.m_base_address);
    std::swap(a.m_size, b.m_size);
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

private:
  using IteratorStorage = std::tuple<It...>;

  IteratorStorage m_iterators;
  std::reference_wrapper<Container> m_container;
  const void *m_base_address;
  size_t m_size;

  void expect_container_unchanged() const {
    if (m_container.get().data() != m_base_address ||
        m_container.get().size() != m_size) {
      throw std::runtime_error("dictionary changed size during iteration");
    }
  }
};

template <class BaseIterator, class Func>
class TransformIterator : public BaseIterator {
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

  template <class F> auto transform(F &&func) const & {
    return BaseIterator::transform(
        [new_f = std::forward<F>(func), old_f = this->m_func](const auto &x) {
          return new_f(old_f(x));
        });
  }

  template <class F> auto transform(F &&func) && {
    // Make a copy for old_f to avoid referencing a member of *this.
    return BaseIterator::transform(
        [new_f = std::forward<F>(func), old_f = this->m_func](const auto &x) {
          return new_f(old_f(x));
        });
  }

private:
  std::decay_t<Func> m_func;
};

template <class I, class F>
TransformIterator(I, F) -> TransformIterator<std::decay_t<I>, std::decay_t<F>>;
} // namespace scipp::core::dict_detail

namespace std {
template <class Container, class... It>
struct iterator_traits<scipp::core::dict_detail::Iterator<Container, It...>> {
private:
  using I = scipp::core::dict_detail::Iterator<Container, It...>;

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
template <class Key, class Value> class Dict {
  using Keys = std::vector<Key>;
  using Values = std::vector<Value>;

public:
  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<const Key, Value>;
  using value_iterator =
      typename dict_detail::Iterator<Values, typename Values::iterator>;
  using iterator =
      typename dict_detail::Iterator<Keys, typename Keys::const_iterator,
                                     typename Values::iterator>;
  using const_key_iterator =
      typename dict_detail::Iterator<const Keys, typename Keys::const_iterator>;
  using const_value_iterator =
      typename dict_detail::Iterator<const Values,
                                     typename Values::const_iterator>;
  using const_iterator =
      typename dict_detail::Iterator<const Keys, typename Keys::const_iterator,
                                     typename Values::const_iterator>;

  Dict(std::initializer_list<std::pair<const Key, Value>> items) {
    reserve(items.size());
    for (const auto &[key, value] : items) {
      if (contains(key))
        throw std::invalid_argument("duplicate key in initializer");
      insert_or_assign(key, value);
    }
  }

  Dict() = default;

  /// Return the number of elements.
  [[nodiscard]] index size() const noexcept { return scipp::size(m_keys); }
  /// Return true if there are 0 elements.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }
  /// Return the number of elements that space is currently allocated for.
  [[nodiscard]] index capacity() const noexcept { return m_keys.capacity(); }

  void reserve(const index new_capacity) {
    m_keys.reserve(new_capacity);
    m_values.reserve(new_capacity);
  }

  [[nodiscard]] bool contains(const Key &key) const noexcept {
    return find_key(key) != m_keys.end();
  }

  template <class V> void insert_or_assign(const key_type &key, V &&value) {
    if (const auto key_it = find_key(key); key_it == m_keys.end()) {
      m_keys.push_back(key);
      m_values.emplace_back(std::forward<V>(value));
    } else {
      m_values[index_of(key_it)] = std::forward<V>(value);
    }
  }

  void erase(const key_type &key) { static_cast<void>(extract(key)); }

  mapped_type extract(const key_type &key) {
    const auto key_it = expect_find_key(key);
    const auto value_it = std::next(m_values.begin(), index_of(key_it));
    m_keys.erase(key_it);
    mapped_type value = std::move(*value_it);
    m_values.erase(value_it);
    return value;
  }

  void clear() {
    m_keys.clear();
    m_values.clear();
  }

  [[nodiscard]] const mapped_type &operator[](const key_type &key) const {
    return m_values[expect_find_index(key)];
  }

  [[nodiscard]] mapped_type &operator[](const key_type &key) {
    return m_values[expect_find_index(key)];
  }

  [[nodiscard]] const mapped_type &at(const key_type &key) const {
    return (*this)[key];
  }

  [[nodiscard]] mapped_type &at(const key_type &key) { return (*this)[key]; }

  [[nodiscard]] const_iterator find(const key_type &key) const {
    if (const auto key_it = find_key(key); key_it == m_keys.end()) {
      return end();
    } else {
      return const_iterator(m_keys, key_it,
                            std::next(m_values.begin(), index_of(key_it)));
    }
  }

  [[nodiscard]] iterator find(const key_type &key) {
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
    return const_value_iterator(m_values, m_values.cbegin());
  }

  [[nodiscard]] auto values_end() const noexcept {
    return const_value_iterator(m_values, m_values.cend());
  }

  [[nodiscard]] auto begin() noexcept {
    return iterator(m_keys, m_keys.cbegin(), m_values.begin());
  }

  [[nodiscard]] auto end() noexcept {
    return iterator(m_keys, m_keys.cend(), m_values.end());
  }

  [[nodiscard]] auto begin() const noexcept {
    return const_iterator(m_keys, m_keys.cbegin(), m_values.cbegin());
  }

  [[nodiscard]] auto end() const noexcept {
    return const_iterator(m_keys, m_keys.cend(), m_values.cbegin());
  }

private:
  Keys m_keys;
  Values m_values;

  auto find_key(const Key &key) const noexcept {
    return std::find(m_keys.begin(), m_keys.end(), key);
  }

  auto expect_find_key(const Key &key) const {
    if (const auto key_it = find_key(key); key_it != m_keys.end()) {
      return key_it;
    }
    using scipp::core::to_string;
    using std::to_string;
    throw except::NotFoundError("Expected " + dict_keys_to_string(*this) +
                                " to contain " + to_string(key) + ".");
  }

  auto index_of(const typename Keys::const_iterator &it) const noexcept {
    return std::distance(m_keys.begin(), it);
  }

  scipp::index expect_find_index(const Key &key) const {
    return index_of(expect_find_key(key));
  }
};

template <class It>
std::string dict_keys_to_string(It it, It end,
                                const std::string_view &dict_name);

template <class Key, class Value>
std::string dict_keys_to_string(const Dict<Key, Value> &dict,
                                const std::string_view &dict_name = "Dict") {
  return dict_keys_to_string(dict.keys_begin(), dict.keys_end(), dict_name);
}
} // namespace scipp::core
