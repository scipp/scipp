// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_ELEMENT_ARRAY_H
#define SCIPP_CORE_ELEMENT_ARRAY_H

#include <memory>

#include "scipp/core/aligned_allocator.h"

namespace scipp::core::detail {

template <class T> class element_array {
public:
  element_array() noexcept = default;
  template <class InputIt> element_array(InputIt first, InputIt last) {
    resize_no_init(std::distance(first, last));
    std::copy(first, last, data());
  }

  explicit operator bool() const noexcept { return m_data.operator bool(); }
  scipp::index size() const noexcept { return m_size; }
  const T *data() const noexcept { return m_data.get(); }
  T *data() noexcept { return m_data.get(); }

  void resize_no_init(const scipp::index new_size) {
    if (new_size != size()) {
      m_data = std::make_unique<T[]>(new_size);
      m_size = new_size;
    }
  }

private:
  scipp::index m_size{-1};
  std::unique_ptr<T[]> m_data;
};
}

#endif // SCIPP_CORE_ELEMENT_ARRAY_H
