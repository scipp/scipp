// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <memory>

#include "scipp/common/index.h"
#include "scipp/core/parallel.h"

namespace scipp::core {

/// Replacement for C++20 std::make_unique_default_init
template <class T> auto make_unique_default_init(const scipp::index size) {
  return std::unique_ptr<T>(new std::remove_extent_t<T>[size]);
}

/// Tag for requesting default-initialization in methods of class element_array.
struct default_init_elements_t {};
static constexpr auto default_init_elements = default_init_elements_t{};

/// Internal data container for Variable.
///
/// This provides a vector-like storage for arrays of elements in a variable.
/// The reasons for not using std::vector are:
/// - Avoiding the std::vector<bool> specialization which would cause issues
///   with thread-safety.
/// - Support default-initialized arrays as an internal optimization in
///   implementing transform. This avoids costly initialization in cases where
///   data would be immediately overwritten afterwards.
/// - As a minor benefit, since the implementation has to store a pointer and a
///   size, we can at the same time support an "optional" behavior, as used for
///   the array of variances in a variable.
template <class T> class element_array {
public:
  using value_type = T;

  element_array() noexcept = default;

  explicit element_array(const scipp::index new_size, const T &value = T()) {
    resize(new_size, default_init_elements);
    parallel::parallel_for(
        parallel::blocked_range(0, size()), [&](const auto &range) {
          std::fill(data() + range.begin(), data() + range.end(), value);
        });
  }

  /// Construct with default-initialized elements. Use with care.
  element_array(const scipp::index new_size, const default_init_elements_t &) {
    resize(new_size, default_init_elements);
  }

  template <
      class Iter,
      std::enable_if_t<
          std::is_assignable<T &, decltype(*std::declval<Iter>())>{}, int> = 0>
  element_array(Iter first, Iter last) {
    const scipp::index size = std::distance(first, last);
    resize(size, default_init_elements);
    parallel::parallel_for(
        parallel::blocked_range(0, size), [&](const auto &range) {
          std::copy(first + range.begin(), first + range.end(),
                    data() + range.begin());
        });
  }

  template <
      class Container,
      std::enable_if_t<
          std::is_assignable_v<T &, typename Container::value_type>, int> = 0>
  explicit element_array(const Container &c)
      : element_array(c.begin(), c.end()) {}

  element_array(std::initializer_list<T> init)
      : element_array(init.begin(), init.end()) {}

  element_array(element_array &&other) noexcept
      : m_size(other.m_size), m_data(std::move(other.m_data)) {
    other.m_size = -1;
  }

  element_array(const element_array &other)
      : element_array(from_other(other)) {}

  element_array &operator=(element_array &&other) noexcept {
    m_data = std::move(other.m_data);
    m_size = other.m_size;
    other.m_size = -1;
    return *this;
  }

  element_array &operator=(const element_array &other) {
    return *this = element_array(other.begin(), other.end());
  }

  explicit operator bool() const noexcept { return m_size != -1; }
  scipp::index size() const noexcept { return m_size; }
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }
  const T *data() const noexcept { return m_data.get(); }
  T *data() noexcept { return m_data.get(); }
  const T *begin() const noexcept { return data(); }
  const T *end() const noexcept {
    return m_size < 0 ? begin() : data() + size();
  }

  void reset() noexcept {
    m_data.reset();
    m_size = -1;
  }

  /// Resize the array.
  ///
  /// Unlike std::vector::resize, this does *not* preserve existing element
  /// values.
  void resize(const scipp::index new_size) { *this = element_array(new_size); }

  /// Resize with default-initialized elements. Use with care.
  void resize(const scipp::index new_size, const default_init_elements_t &) {
    if (new_size == 0) {
      m_data.reset();
      m_size = 0;
    } else if (new_size != size()) {
      m_data = make_unique_default_init<T[]>(new_size);
      m_size = new_size;
    }
  }

private:
  element_array from_other(const element_array &other) {
    if (other.size() == -1) {
      return element_array();
    } else if (other.size() == 0) {
      return element_array(0);
    } else {
      return element_array(other.begin(), other.end());
    }
  }
  scipp::index m_size{-1};
  std::unique_ptr<T[]> m_data;
};

} // namespace scipp::core

namespace scipp {
using core::element_array;
}
