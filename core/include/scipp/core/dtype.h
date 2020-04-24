// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <typeindex>

#include <boost/container/small_vector.hpp>

#include "scipp-core_export.h"
#include "scipp/common/span.h"

namespace scipp::core {

template <class T> using event_list = boost::container::small_vector<T, 8>;

template <class T> struct is_events : std::false_type {};
template <class T> struct is_events<event_list<T>> : std::true_type {};
template <class T> struct is_events<event_list<T> &> : std::true_type {};
template <class T> struct is_events<const event_list<T> &> : std::true_type {};
template <class T> inline constexpr bool is_events_v = is_events<T>::value;

struct SCIPP_CORE_EXPORT DType {
  std::type_index index;
  bool operator==(const DType &t) const noexcept { return index == t.index; }
  bool operator!=(const DType &t) const noexcept { return index != t.index; }
  bool operator<(const DType &t) const noexcept { return index < t.index; }
};
template <class T> inline DType dtype{std::type_index(typeid(T))};

bool isInt(DType tp);

DType event_dtype(const DType type);

namespace detail {
template <class T> struct element_type { using type = T; };
template <class T> struct element_type<event_list<T>> { using type = T; };
template <class T> struct element_type<const event_list<T>> { using type = T; };
template <class T> using element_type_t = typename element_type<T>::type;
} // namespace detail

template <class T> constexpr bool canHaveVariances() noexcept {
  using U = std::remove_const_t<T>;
  return std::is_same_v<U, double> || std::is_same_v<U, float> ||
         std::is_same_v<U, event_list<double>> ||
         std::is_same_v<U, event_list<float>> ||
         std::is_same_v<U, span<const double>> ||
         std::is_same_v<U, span<const float>> ||
         std::is_same_v<U, span<double>> || std::is_same_v<U, span<float>>;
}

} // namespace scipp::core

namespace scipp {
using core::DType;
using core::dtype;
using core::event_list;
} // namespace scipp
