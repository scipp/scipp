// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <Eigen/Dense>

#include "scipp/core/transform_common.h"

namespace scipp::core::element {

struct plus_equals {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const
      noexcept(noexcept(a += b)) {
    a += b;
  }
  using types = decltype(std::tuple_cat(
      pair_self_t<double, float, int32_t, int64_t, Eigen::Vector3d>{},
      pair_custom_t<std::tuple<double, float>,
                    std::tuple<int64_t, int32_t>>{}));
};
struct minus_equals {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const
      noexcept(noexcept(a -= b)) {
    a -= b;
  }
  using types = decltype(std::tuple_cat(
      pair_self_t<double, float, int32_t, int64_t, Eigen::Vector3d>{},
      pair_custom_t<std::tuple<double, float>,
                    std::tuple<int64_t, int32_t>>{}));
};
struct times_equals {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const
      noexcept(noexcept(a *= b)) {
    a *= b;
  }
  using types = decltype(std::tuple_cat(
      pair_self_t<double, float, int32_t, int64_t>{},
      pair_custom_t<std::tuple<double, float>, std::tuple<float, double>,
                    std::tuple<int64_t, int32_t>,
                    std::tuple<Eigen::Vector3d, double>>{},
      pair_numerical_with_t<bool>{}));
};
struct divide_equals {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const
      noexcept(noexcept(a /= b)) {
    a /= b;
  }
  using types = decltype(std::tuple_cat(
      pair_self_t<double, float, int32_t, int64_t>{},
      pair_custom_t<std::tuple<double, float>, std::tuple<int64_t, int32_t>,
                    std::tuple<Eigen::Vector3d, double>>{}));
};

} // namespace scipp::core::element
