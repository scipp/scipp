// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

namespace scipp::core {

template <class T, class = void> struct has_eval : std::false_type {};
template <class T>
struct has_eval<T, std::void_t<decltype(std::declval<T>().eval())>>
    : std::true_type {};

/// True if T has an eval() method. Used by `transform` to detect expression
/// templates (from Eigen).
template <class T> inline constexpr bool has_eval_v = has_eval<T>::value;

} // namespace scipp::core
