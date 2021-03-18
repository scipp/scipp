// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <tuple>

namespace scipp::common {

// https://stackoverflow.com/a/18063608
template <class T, class Tuple> struct index_in_tuple;

template <class T, class... Types>
struct index_in_tuple<T, std::tuple<T, Types...>> {
  static const std::size_t value = 0;
};

template <class T, class U, class... Types>
struct index_in_tuple<T, std::tuple<U, Types...>> {
  static const std::size_t value =
      1 + index_in_tuple<T, std::tuple<Types...>>::value;
};

template <class, class = void> struct has_const_view_type : std::false_type {};

template <class T>
struct has_const_view_type<T, std::void_t<typename T::const_view_type>>
    : std::true_type {};

/// true iff T has a const view, e.g. Variable or ConstVariableView.
template <class T>
constexpr inline bool has_const_view_type_v = has_const_view_type<T>::value;

template <class, class = void> struct is_const_view_type : std::false_type {};

// All const views T have a const_view_type member which is equal to T itself.
template <class T>
struct is_const_view_type<T, std::enable_if_t<has_const_view_type_v<T>>>
    : std::is_same<typename T::const_view_type, std::decay_t<T>> {};

/// true iff T is a const view, e.g. ConstVariableView.
template <class T>
constexpr inline bool is_const_view_type_v = is_const_view_type<T>::value;

} // namespace scipp::common
