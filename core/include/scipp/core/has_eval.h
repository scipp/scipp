// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

namespace scipp::core {

template <class T> struct has_eval {
  template <class U>
  static auto test(int) -> decltype(std::declval<U>().eval(), std::true_type());
  template <class> static std::false_type test(...);
  static constexpr bool value =
      std::is_same<decltype(test<T>(0)), std::true_type>::value;
};

/// True if T has an eval() method. Used by `transform` to detect expression
/// templates (from Eigen).
template <class T> inline constexpr bool has_eval_v = has_eval<T>::value;

} // namespace scipp::core
