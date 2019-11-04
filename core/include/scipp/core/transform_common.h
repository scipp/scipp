// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_TRANSFORM_COMMON_H
#define SCIPP_CORE_TRANSFORM_COMMON_H

#include <tuple>

namespace scipp::core {

template <class... Ts> struct pair_self {
  using type = std::tuple<std::pair<Ts, Ts>...>;
};
template <class... Ts> struct pair_custom { using type = std::tuple<Ts...>; };
template <class... Ts> struct pair_ {
  template <class RHS> struct with {
    using type = decltype(std::tuple_cat(std::tuple<std::pair<Ts, RHS>>{}...));
  };
};

template <class... Ts> using pair_self_t = typename pair_self<Ts...>::type;
template <class... Ts> using pair_custom_t = typename pair_custom<Ts...>::type;
template <class RHS>
using pair_numerical_with_t =
    typename pair_<double, float, int64_t, int32_t>::with<RHS>::type;

} // namespace scipp::core

#endif // SCIPP_CORE_TRANSFORM_COMMON_H
