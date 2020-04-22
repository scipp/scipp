// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// Originally from https://stackoverflow.com/a/38279655
#pragma once

#include <iostream>
#include <type_traits>

#include <boost/mpl/at.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/is_sequence.hpp>

#include <boost/units/io.hpp>
#include <boost/units/systems/si.hpp>

using namespace boost::units;

template <typename T>
using get_dimension_t = typename T::unit_type::dimension_type;

template <typename T> struct get_tag { using type = typename T::tag_type; };

template <typename T> using get_tag_t = typename T::tag_type;

template <class...> using void_t = void;

template <typename U, typename V> struct Exponent {
  template <typename Dim, typename Enable = void> struct get {
    static constexpr int value = 0;
  };

  template <typename Dim> struct get<Dim, void_t<typename Dim::value_type>> {
    using Value = typename Dim::value_type;
    static constexpr int value = Value::Numerator / Value::Denominator;
  };

  using dimension_V = get_dimension_t<V>;

  using tag_to_search_for =
      typename get_tag<typename boost::mpl::at_c<dimension_V, 0>::type>::type;

  using dimension_U = get_dimension_t<U>;

  using iter = typename boost::mpl::find_if<
      dimension_U,
      std::is_same<get_tag<boost::mpl::_1>, tag_to_search_for>>::type;

  using Dim = typename boost::mpl::deref<iter>::type;

  constexpr static int value = get<Dim>::value;
};

template <typename U, typename V> constexpr auto getExponent(U &&, V &&) {
  return Exponent<std::decay_t<U>, std::decay_t<V>>::value;
}
