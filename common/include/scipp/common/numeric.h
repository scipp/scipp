// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "scipp/common/index.h"

namespace scipp::numeric {

template <class Range> bool islinspace(const Range &range) {
  if (scipp::size(range) < 2)
    return false;
  if (range.back() <= range.front())
    return false;

  using T = typename Range::value_type;
  if constexpr (std::is_floating_point_v<T>) {
    const T delta = (range.back() - range.front()) / (scipp::size(range) - 1);
    constexpr int32_t ulp = 4;
    const T epsilon = std::numeric_limits<T>::epsilon() *
                      (std::abs(range.front()) + std::abs(range.back())) * ulp;
    return std::adjacent_find(range.begin(), range.end(),
                              [epsilon, delta](const auto &a, const auto &b) {
                                return std::abs(std::abs(b - a) - delta) >
                                       epsilon;
                              }) == range.end();
  } else {
    const auto delta = range[1] - range[0];
    return std::adjacent_find(range.begin(), range.end(),
                              [delta](const auto &a, const auto &b) {
                                return std::abs(b - a) != delta;
                              }) == range.end();
  }
}

// Division like Python's __truediv__
template <class T, class U> auto true_divide(const T &a, const U &b) {
  if constexpr (std::is_integral_v<T> && std::is_integral_v<U>)
    return static_cast<double>(a) / static_cast<double>(b);
  else
    return a / b;
}

// Division like Python's __floordiv__
template <class T, class U>
std::common_type_t<T, U> floor_divide(const T &a, const U &b) {
  using std::floor;
  if constexpr (std::is_integral_v<T> && std::is_integral_v<U>)
    return b == 0 ? 0 : floor(static_cast<double>(a) / static_cast<double>(b));
  else
    return floor(a / b);
}

// Remainder like Python's __mod__, complementary to floor_divide.
template <class T, class U> auto remainder(const T &a, const U &b) {
  if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
    return b == 0 ? NAN : a - floor_divide(a, b) * b;
  } else {
    return b == 0 ? 0 : a - floor_divide(a, b) * b;
  }
}

namespace {
template <class B, class E>
constexpr auto integer_pow_pos_exponent(const B &base,
                                        const E exponent) noexcept {
  static_assert(std::is_integral_v<std::decay_t<E>>);

  if (exponent == 0)
    return static_cast<B>(1);
  if (exponent == 1)
    return base;

  const auto aux = integer_pow_pos_exponent(base, exponent / 2);
  if (exponent % 2 == 0)
    return aux * aux;
  return base * aux * aux;
}
} // namespace

template <class B, class E>
constexpr auto pow(const B base, const E exponent) noexcept {
  if constexpr (std::is_integral_v<std::decay_t<E>>) {
    if (exponent >= 0) {
      return integer_pow_pos_exponent(base, exponent);
    } else {
      return static_cast<std::decay_t<B>>(1) /
             integer_pow_pos_exponent(base, -exponent);
    }
  } else {
    using std::pow;
    return pow(base, exponent);
  }
}

template <class T> bool isnan([[maybe_unused]] T x) {
  if constexpr (std::is_floating_point_v<std::decay_t<T>>)
    return std::isnan(x);
  else
    return false;
}

template <class T> bool isinf([[maybe_unused]] T x) {
  if constexpr (std::is_floating_point_v<std::decay_t<T>>)
    return std::isinf(x);
  else
    return false;
}

template <class T> bool isfinite([[maybe_unused]] T x) {
  if constexpr (std::is_floating_point_v<std::decay_t<T>>)
    return std::isfinite(x);
  else
    return true;
}

template <class T> bool signbit([[maybe_unused]] T x) {
  if constexpr (std::is_floating_point_v<std::decay_t<T>>)
    return std::signbit(x);
  else
    return std::signbit(double(x));
}

} // namespace scipp::numeric
