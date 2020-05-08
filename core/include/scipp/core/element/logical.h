// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/transform_common.h"

namespace scipp::core::element {

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

} // namespace scipp::core::element
