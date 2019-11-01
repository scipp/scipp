// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_ELEMENT_ARRAY_H
#define SCIPP_CORE_ELEMENT_ARRAY_H

#include <memory>

#include "scipp/common/index.h"
#include "scipp/core/aligned_allocator.h"

namespace scipp::core::detail {

/// Replacement for C++20 std::make_unique_default_init
template <class T> auto make_unique_default_init(const scipp::index size) {
  return std::unique_ptr<T>(new std::remove_extent_t<T>[size]);
}

template <class T> class element_array {
public:
  element_array() noexcept = default;
  template <class InputIt> element_array(InputIt first, InputIt last) {
    resize_no_init(std::distance(first, last));
    std::copy(first, last, data());
  }
  element_array(element_array &&other) = default;
  element_array(const element_array &other)
      : element_array(other.data(), other.data() + other.size()) {}
  element_array &operator=(element_array &&other) = default;
  element_array &operator=(const element_array &other) {
    return *this = element_array(other.data(), other.data() + other.size());
  }

  explicit operator bool() const noexcept { return m_data.operator bool(); }
  scipp::index size() const noexcept { return m_size; }
  const T *data() const noexcept { return m_data.get(); }
  T *data() noexcept { return m_data.get(); }

  void reset() noexcept {
    m_data.reset();
    m_size = -1;
  }

  void resize_no_init(const scipp::index new_size) {
    if (new_size != size()) {
      m_data = make_unique_default_init<T[]>(new_size);
      m_size = new_size;
    }
  }

  void resize(const scipp::index new_size) {
    m_data = std::make_unique<T[]>(new_size);
    m_size = new_size;
  }

private:
  scipp::index m_size{-1};
  std::unique_ptr<T[]> m_data;
};

} // namespace scipp::core::detail

#endif // SCIPP_CORE_ELEMENT_ARRAY_H
