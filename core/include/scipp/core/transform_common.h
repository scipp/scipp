// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <tuple>

#include "scipp/common/overloaded.h"
#include "scipp/core/except.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/core/values_and_variances.h"

namespace scipp::core {

template <class... Ts> struct pair_self {
  using type = std::tuple<std::tuple<Ts, Ts>...>;
};
template <class... Ts> struct pair_custom { using type = std::tuple<Ts...>; };
template <class... Ts> struct pair_ {
  template <class RHS> using type = std::tuple<std::tuple<Ts, RHS>...>;
};

template <class... Ts> using pair_self_t = typename pair_self<Ts...>::type;
template <class... Ts> using pair_custom_t = typename pair_custom<Ts...>::type;
template <class RHS>
using pair_numerical_with_t =
    typename pair_<double, float, int64_t, int32_t>::type<RHS>;

template <class... Ts> struct pair_product {
  template <class T> using type = std::tuple<std::tuple<T, Ts>...>;
};

template <class... Ts>
using pair_product_t = decltype(
    std::tuple_cat(typename pair_product<Ts...>::template type<Ts>{}...));

using arithmetic_type_pairs = pair_product_t<float, double, int32_t, int64_t>;

using arithmetic_type_pairs_with_bool =
    decltype(std::tuple_cat(std::declval<arithmetic_type_pairs>(),
                            std::declval<pair_numerical_with_t<bool>>()));

using arithmetic_and_matrix_type_pairs = decltype(
    std::tuple_cat(std::declval<arithmetic_type_pairs>(),
                   std::tuple<std::tuple<Eigen::Vector3d, Eigen::Vector3d>>()));

static constexpr auto dimensionless_unit_check =
    [](units::Unit &varUnit, const units::Unit &otherUnit) {
      expect::equals(varUnit, units::dimensionless);
      expect::equals(otherUnit, units::dimensionless);
    };

static constexpr auto dimensionless_unit_check_return =
    overloaded{[](const units::Unit &a) {
                 expect::equals(a, units::one);
                 return units::one;
               },
               [](const units::Unit &a, const units::Unit &b) {
                 expect::equals(a, units::one);
                 expect::equals(b, units::one);
                 return units::one;
               }};

/// Flags for transform, added as overloads to the operator. These are never
/// actually called since flag presence is checked via the base class of the
/// operator.
namespace transform_flags {
/// Add this to overloaded operator to indicate that the operation does not
/// return data with variances, regardless of whether inputs have variances.
static constexpr auto no_out_variance = []() {};
using no_out_variance_t = decltype(no_out_variance);

/// Add this to overloaded operator to indicate that the operation does not
/// support variances in the specified argument.
template <int N> static constexpr auto expect_no_variance_arg = []() {};
template <int N>
using expect_no_variance_arg_t = decltype(expect_no_variance_arg<N>);

/// Add this to overloaded operator to indicate that the operation requires
/// variances in the specified argument.
template <int N> static constexpr auto expect_variance_arg = []() {};
template <int N> using expect_variance_arg_t = decltype(expect_variance_arg<N>);

/// Add this to overloaded operator to indicate that the in-place operation
/// requires inputs to have a variance if the output has a variance.
static constexpr auto expect_in_variance_if_out_variance = []() {};
using expect_in_variance_if_out_variance_t =
    decltype(expect_in_variance_if_out_variance);

static constexpr auto expect_all_or_none_have_variance = []() {};
using expect_all_or_none_have_variance_t =
    decltype(expect_all_or_none_have_variance);

} // namespace transform_flags

} // namespace scipp::core
