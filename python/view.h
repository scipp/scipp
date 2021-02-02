// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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

static constexpr auto dim_to_str = [](auto &&dim) -> decltype(auto) {
  return dim.name();
};

/// Helper to provide equivalent of the `keys()` method of a Python dict.
template <class T> class str_keys_view {
public:
  str_keys_view(T &obj) : m_obj(&obj) {}
  auto size() const noexcept { return m_obj->size(); }
  auto begin() const {
    return boost::make_transform_iterator(m_obj->keys_begin(), dim_to_str);
  }
  auto end() const {
    return boost::make_transform_iterator(m_obj->keys_end(), dim_to_str);
  }

private:
  T *m_obj;
};
template <class T> str_keys_view(T &) -> str_keys_view<T>;

static constexpr auto item_to_str = [](auto &&item) -> decltype(auto) {
  return std::make_pair<std::string, decltype(item.second)>(
      item.first.name(), std::move(item.second));
};

/// Helper to provide equivalent of the `items()` method of a Python dict.
template <class T> class str_items_view {
public:
  str_items_view(T &obj) : m_obj(&obj) {}
  auto size() const noexcept { return m_obj->size(); }
  auto begin() const {
    return boost::make_transform_iterator(m_obj->items_begin(), item_to_str);
  }
  auto end() const {
    return boost::make_transform_iterator(m_obj->items_end(), item_to_str);
  }

private:
  T *m_obj;
};
template <class T> str_items_view(T &) -> str_items_view<T>;
