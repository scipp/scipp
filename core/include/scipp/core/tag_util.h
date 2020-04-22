// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <array>

#include "scipp/core/dtype.h"

namespace scipp::core {

/**
 * At the time of writing older clang 6 lacks support for template
 * argument deduction of class templates (OSX only?)
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0620r0.html
 * https://clang.llvm.org/cxx_status.html
 *
 * This make helper uses function template argument deduction to solve
 * for std::array declarations.
 */
template <typename... T> constexpr auto make_array(T &&... values) {
  return std::array<std::decay_t<std::common_type_t<T...>>, sizeof...(T)>{
      std::forward<T>(values)...};
}

template <template <class> class Callable, class... Ts, class... Args>
static auto callDType(const std::tuple<Ts...> &, const DType dtype,
                      Args &&... args) {
  auto funcs = make_array(Callable<Ts>::apply...);
  std::array<DType, sizeof...(Ts)> dtypes{scipp::core::dtype<Ts>...};
  for (size_t i = 0; i < dtypes.size(); ++i)
    if (dtypes[i] == dtype)
      return funcs[i](std::forward<Args>(args)...);
  throw std::runtime_error("Unsupported dtype.");
}

/// Apply Callable<T> to args, for any dtype T in Ts, determined by the
/// runtime dtype given by `dtype`.
template <class... Ts> struct CallDType {
  template <template <class> class Callable, class... Args>
  static auto apply(const DType dtype, Args &&... args) {
    return callDType<Callable>(std::tuple<Ts...>{}, dtype,
                               std::forward<Args>(args)...);
  }
};

} // namespace scipp::core
