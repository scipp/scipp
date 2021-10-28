// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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

static constexpr auto keep_unit =
    overloaded{[](const units::Unit &) {},
               [](const units::Unit &, const units::Unit &) {}};

static constexpr auto dimensionless_unit_check =
    [](units::Unit &varUnit, const units::Unit &otherUnit) {
      expect::equals(units::one, varUnit);
      expect::equals(units::one, otherUnit);
    };

static constexpr auto dimensionless_unit_check_return =
    overloaded{[](const units::Unit &a) {
                 expect::equals(units::one, a);
                 return units::one;
               },
               [](const units::Unit &a, const units::Unit &b) {
                 expect::equals(units::one, a);
                 expect::equals(units::one, b);
                 return units::one;
               }};

template <typename Op> struct assign_unary : Op {
  template <typename Out, typename... In>
  void operator()(Out &out, In &&... in) {
    out = Op::operator()(std::forward<In>(in)...);
  }
};
template <typename Op> assign_unary(Op) -> assign_unary<Op>;

/// Flags for transform, added as overloads to the operator. These are never
/// actually called since flag presence is checked via the base class of the
/// operator.
namespace transform_flags {

/// Base and NULL flag. Do not test for this type.
struct Flag {
  void operator()() const {};
};

/// Helper to conditionally apply a given flag under condition (B) otherwise
/// Null Flag
template <bool B>
constexpr auto conditional_flag =
    [](auto flag) { return std::conditional_t<B, decltype(flag), Flag>{}; };

namespace {

struct no_out_variance_t : Flag {};
/// Add this to overloaded operator to indicate that the operation does not
/// return data with variances, regardless of whether inputs have variances.
constexpr auto no_out_variance = no_out_variance_t{};

template <int N> struct expect_no_variance_arg_t : Flag {};
/// Add this to overloaded operator to indicate that the operation does not
/// support variances in the specified argument.
template <int N>
constexpr auto expect_no_variance_arg = expect_no_variance_arg_t<N>{};

struct expect_no_in_variance_if_out_cannot_have_variance_t : Flag {};
/// Add this to overloaded operator to indicate that if the output dtype
/// does not support variance none of the inputs should have a variance.
constexpr auto expect_no_in_variance_if_out_cannot_have_variance =
    expect_no_in_variance_if_out_cannot_have_variance_t{};

template <int N> struct expect_variance_arg_t : Flag {};
/// Add this to overloaded operator to indicate that the operation requires
/// variances in the specified argument.
template <int N>
constexpr auto expect_variance_arg = expect_variance_arg_t<N>{};

struct expect_in_variance_if_out_variance_t : Flag {};
/// Add this to overloaded operator to indicate that the in-place operation
/// requires inputs to have a variance if the output has a variance.
constexpr auto expect_in_variance_if_out_variance =
    expect_in_variance_if_out_variance_t{};

struct expect_all_or_none_have_variance_t : Flag {};
constexpr auto expect_all_or_none_have_variance =
    expect_all_or_none_have_variance_t{};

} // namespace
} // namespace transform_flags

} // namespace scipp::core
