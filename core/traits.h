/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef TRAITS_H
#define TRAITS_H

#include <type_traits>

namespace scipp::core {

class Dataset;
template <class D, class... Ts> class MDZipViewImpl;

namespace detail {
template <class T, class Tuple> struct index;
template <class T, class... Types> struct index<T, std::tuple<T, Types...>> {
  static const std::size_t value = 0;
};
template <class T, class U, class... Types>
struct index<T, std::tuple<U, Types...>> {
  static const std::size_t value = 1 + index<T, std::tuple<Types...>>::value;
};

template <class... Conds> struct and_ : std::true_type {};
template <class Cond, class... Conds>
struct and_<Cond, Conds...>
    : std::conditional<Cond::value, and_<Conds...>, std::false_type>::type {};

// Cannot use std::is_const because we need to handle the special case of a
// nested MDZipView.
template <class T> struct is_const : std::false_type {};
template <class T> struct is_const<const T> : std::true_type {};

template <class D, class... Ts>
struct is_const<MDZipViewImpl<D, Ts...>> : and_<is_const<Ts>...> {};
} // namespace detail

} // namespace scipp::core

#endif // TRAITS_H
