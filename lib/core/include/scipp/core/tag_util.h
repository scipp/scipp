// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <array>

#include "scipp/core/dtype.h"
#include "scipp/core/except.h"

namespace scipp::core {

template <template <class> class Callable, class... Ts, class... Args>
static auto callDType(const std::tuple<Ts...> &, const DType dtype,
                      Args &&... args) {
  std::array funcs{Callable<Ts>::apply...};
  std::array<DType, sizeof...(Ts)> dtypes{scipp::core::dtype<Ts>...};
  for (size_t i = 0; i < dtypes.size(); ++i)
    if (dtypes[i] == dtype)
      return funcs[i](std::forward<Args>(args)...);
  throw except::TypeError("Unsupported dtype.");
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
