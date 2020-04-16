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

template <class T>
using sparse_container = boost::container::small_vector<T, 8>;
template <class T> using event_list = sparse_container<T>;

template <class T> struct is_sparse : std::false_type {};
template <class T> struct is_sparse<sparse_container<T>> : std::true_type {};
template <class T> struct is_sparse<sparse_container<T> &> : std::true_type {};
template <class T>
struct is_sparse<const sparse_container<T> &> : std::true_type {};
template <class T> inline constexpr bool is_sparse_v = is_sparse<T>::value;

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
template <class T> struct element_type<sparse_container<T>> { using type = T; };
template <class T> struct element_type<const sparse_container<T>> {
  using type = T;
};
template <class T> using element_type_t = typename element_type<T>::type;
} // namespace detail

template <class T> constexpr bool canHaveVariances() noexcept {
  using U = std::remove_const_t<T>;
  return std::is_same_v<U, double> || std::is_same_v<U, float> ||
         std::is_same_v<U, sparse_container<double>> ||
         std::is_same_v<U, sparse_container<float>> ||
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
