// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_ELEMENT_ARRAY_H
#define SCIPP_CORE_ELEMENT_ARRAY_H

#include <algorithm>
#include <memory>

#include "scipp/common/index.h"
#include "scipp/core/aligned_allocator.h"

namespace scipp::core::detail {

/// Replacement for C++20 std::make_unique_default_init
template <class T> auto make_unique_default_init(const scipp::index size) {
  return std::unique_ptr<T>(new std::remove_extent_t<T>[size]);
}

static constexpr auto default_init_elements = []() {};
using default_init_elements_t = decltype(default_init_elements);

template <class T> class element_array {
public:
  using value_type = T;

  element_array() noexcept = default;

  explicit element_array(const scipp::index new_size, const T &value = T()) {
    resize(new_size, default_init_elements);
    std::fill(data(), data() + size(), value);
  }

  element_array(const scipp::index new_size, const default_init_elements_t &) {
    resize(new_size, default_init_elements);
  }

  template <class InputIt,
            std::enable_if_t<!std::is_integral<InputIt>{}, int> = 0>
  element_array(InputIt first, InputIt last) {
    resize(std::distance(first, last), default_init_elements);
    std::copy(first, last, data());
  }

  element_array(std::initializer_list<T> init)
      : element_array(init.begin(), init.end()) {}

  element_array(element_array &&other) noexcept
      : m_size(other.m_size), m_data(std::move(other.m_data)) {
    other.m_size = -1;
  }

  element_array(const element_array &other)
      : element_array(other.data(), other.data() + other.size()) {}

  element_array &operator=(element_array &&other) noexcept {
    m_data = std::move(other.m_data);
    m_size = other.m_size;
    other.m_size = -1;
    return *this;
  }

  element_array &operator=(const element_array &other) {
    return *this = element_array(other.data(), other.data() + other.size());
  }

  explicit operator bool() const noexcept { return m_size != -1; }
  scipp::index size() const noexcept { return m_size; }
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }
  const T *data() const noexcept { return m_data.get(); }
  T *data() noexcept { return m_data.get(); }
  const T *begin() const noexcept { return data(); }
  const T *end() const noexcept { return data() + size(); }

  void reset() noexcept {
    m_data.reset();
    m_size = -1;
  }

  void resize(const scipp::index new_size) {
    if (new_size == 0) {
      m_data.reset();
      m_size = 0;
    } else {
      m_data = std::make_unique<T[]>(new_size);
      m_size = new_size;
    }
  }

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
  scipp::index m_size{-1};
  std::unique_ptr<T[]> m_data;
};

} // namespace scipp::core::detail

#endif // SCIPP_CORE_ELEMENT_ARRAY_H
