// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

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

} // namespace scipp::common
