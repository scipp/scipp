// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "scipp/common/index.h"

namespace scipp::numeric {

template <class Range> bool is_linspace(const Range &range) {
  if (scipp::size(range) < 2)
    return false;
  if (range.back() <= range.front())
    return false;

  using T = typename Range::value_type;
  const T delta = (range.back() - range.front()) / (scipp::size(range) - 1);
  constexpr int32_t ulp = 4;
  const T epsilon = std::numeric_limits<T>::epsilon() *
                    (std::abs(range.front()) + std::abs(range.back())) * ulp;

  return std::adjacent_find(range.begin(), range.end(),
                            [epsilon, delta](const auto &a, const auto &b) {
                              return std::abs(std::abs(b - a) - delta) >
                                     epsilon;
                            }) == range.end();
}

} // namespace scipp::numeric
