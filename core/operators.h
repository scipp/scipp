// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_OPERATORS_H
#define SCIPP_CORE_OPERATORS_H

#include <Eigen/Dense>

namespace scipp::core {

template <class... Ts> struct pair_self {
  using type = std::tuple<std::pair<Ts, Ts>...>;
};
template <class... Ts> struct pair_custom { using type = std::tuple<Ts...>; };

template <class... Ts> using pair_self_t = typename pair_self<Ts...>::type;
template <class... Ts> using pair_custom_t = typename pair_custom<Ts...>::type;

namespace operator_detail {
struct plus_equals {
  template <class A, class B>
  constexpr auto operator()(A &&a, const B &b) const
      noexcept(noexcept(a += b)) {
    return a += b;
  }
  using types = pair_self_t<double, float, int64_t, Eigen::Vector3d>;
};
struct minus_equals {
  template <class A, class B>
  constexpr auto operator()(A &&a, const B &b) const
      noexcept(noexcept(a -= b)) {
    return a -= b;
  }
  using types = pair_self_t<double, float, int64_t, Eigen::Vector3d>;
};
struct times_equals {
  template <class A, class B>
  constexpr auto operator()(A &&a, const B &b) const
      noexcept(noexcept(a *= b)) {
    return a *= b;
  }
  using types = decltype(
      std::tuple_cat(pair_self_t<double, float, int64_t>{},
                     pair_custom_t<std::pair<Eigen::Vector3d, double>>{}));
};
struct divide_equals {
  template <class A, class B>
  constexpr auto operator()(A &&a, const B &b) const
      noexcept(noexcept(a /= b)) {
    return a /= b;
  }
  using types = decltype(
      std::tuple_cat(pair_self_t<double, float, int64_t>{},
                     pair_custom_t<std::pair<Eigen::Vector3d, double>>{}));
};
} // namespace operator_detail

} // namespace scipp::core

#endif // SCIPP_CORE_OPERATORS_H
