// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

/// Helper to provide equivalent of the `items()` method of a Python dict.
template <class T> class items_view {
public:
  items_view(T &obj) : m_obj(&obj) {}
  auto size() const noexcept { return m_obj->size(); }
  auto begin() const { return m_obj->items_begin(); }
  auto end() const { return m_obj->items_end(); }

private:
  T *m_obj;
};
template <class T> items_view(T &) -> items_view<T>;

/// Helper to provide equivalent of the `values()` method of a Python dict.
template <class T> class values_view {
public:
  values_view(T &obj) : m_obj(&obj) {}
  auto size() const noexcept { return m_obj->size(); }
  auto begin() const {
    if constexpr (std::is_same_v<typename T::mapped_type, DataArray>)
      return m_obj->begin();
    else
      return m_obj->values_begin();
  }
  auto end() const {
    if constexpr (std::is_same_v<typename T::mapped_type, DataArray>)
      return m_obj->end();
    else
      return m_obj->values_end();
  }

private:
  T *m_obj;
};
template <class T> values_view(T &) -> values_view<T>;

/// Helper to provide equivalent of the `keys()` method of a Python dict.
template <class T> class keys_view {
public:
  keys_view(T &obj) : m_obj(&obj) {}
  auto size() const noexcept { return m_obj->size(); }
  auto begin() const { return m_obj->keys_begin(); }
  auto end() const { return m_obj->keys_end(); }

private:
  T *m_obj;
};
template <class T> keys_view(T &) -> keys_view<T>;
