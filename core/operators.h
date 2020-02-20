// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_OPERATORS_H
#define SCIPP_CORE_OPERATORS_H

#include <algorithm>

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
                    std::pair<Eigen::Vector3d, double>>{},
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
      pair_custom_t<std::pair<double, float>, std::pair<int64_t, int32_t>,
                    std::pair<Eigen::Vector3d, double>>{}));
};

struct and_equals {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const
      noexcept(noexcept(a &= b)) {
    a &= b;
  }
  using types = pair_self_t<bool>;
};
struct or_equals {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const
      noexcept(noexcept(a |= b)) {
    a |= b;
  }
  using types = pair_self_t<bool>;
};

struct max_equals
    : public transform_flags::expect_in_variance_if_out_variance_t {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const noexcept {
    using std::max;
    a = max(a, b);
  }
  using types = pair_self_t<double, float, int64_t, int32_t>;
};
struct min_equals
    : public transform_flags::expect_in_variance_if_out_variance_t {
  template <class A, class B>
  constexpr void operator()(A &&a, const B &b) const noexcept {
    using std::min;
    a = min(a, b);
  }
  using types = pair_self_t<double, float, int64_t, int32_t>;
};
} // namespace operator_detail

} // namespace scipp::core

#endif // SCIPP_CORE_OPERATORS_H
