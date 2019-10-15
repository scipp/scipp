// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_OPERATORS_H
#define SCIPP_CORE_OPERATORS_H

#include <Eigen/Dense>

#include "scipp/core/transform_common.h"

namespace scipp::core {

namespace operator_detail {
struct plus_equals {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const
      noexcept(noexcept(a += b)) {
    a += b;
  }
  using types = decltype(std::tuple_cat(
      pair_self_t<double, float, int32_t, int64_t, Eigen::Vector3d>{},
      pair_custom_t<std::pair<double, float>, std::pair<int64_t, int32_t>>{}));
};
struct minus_equals {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const
      noexcept(noexcept(a -= b)) {
    a -= b;
  }
  using types = decltype(std::tuple_cat(
      pair_self_t<double, float, int32_t, int64_t, Eigen::Vector3d>{},
      pair_custom_t<std::pair<double, float>, std::pair<int64_t, int32_t>>{}));
};
struct times_equals {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const
      noexcept(noexcept(a *= b)) {
    a *= b;
  }
  using types = decltype(std::tuple_cat(
      pair_self_t<double, float, int32_t, int64_t>{},
      pair_custom_t<std::pair<double, float>, std::pair<float, double>,
                    std::pair<int64_t, int32_t>,
                    std::pair<double, Bool>,
                    std::pair<float, Bool>,
                    std::pair<int64_t, Bool>,
                    std::pair<int32_t, Bool>,
                    std::pair<Eigen::Vector3d, double>>{}));
};
struct divide_equals {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const
      noexcept(noexcept(a /= b)) {
    a /= b;
  }
  using types = decltype(std::tuple_cat(
      pair_self_t<double, float, int32_t, int64_t>{},
      pair_custom_t<std::pair<double, float>, std::pair<int64_t, int32_t>,
                    std::pair<Eigen::Vector3d, double>>{}));
};
} // namespace operator_detail

} // namespace scipp::core

#endif // SCIPP_CORE_OPERATORS_H
