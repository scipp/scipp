// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#ifndef VALUE_AND_VARIANCE_H
#define VALUE_AND_VARIANCE_H

#include "scipp/core/variable.h"

namespace scipp::core {

namespace detail {

/// A value/variance pair with operators that propagate uncertainties.
///
/// This is intended for small T such as double, float, and int. It is the
/// central implementation of uncertainty propagation in scipp, for built-in
/// operations as well as custom operations using one of the transform
/// functions. Since T is assumed to be small it is copied into the class and
/// extracted later. See also ValuesAndVariances.
template <class T> struct ValueAndVariance {
  /// This constructor is essential to prevent warnings about narrowing the
  /// types in initializer lists used below
  template <class T1, class T2>
  constexpr ValueAndVariance(T1 t1, T2 t2) : value(t1), variance(t2) {}
  T value;
  T variance;

  template <class T2>
  constexpr auto &operator=(const ValueAndVariance<T2> other) noexcept {
    value = other.value;
    variance = other.variance;
    return *this;
  }

  template <class T2> constexpr auto &operator=(const T2 other) noexcept {
    value = other;
    variance = 0.0;
    return *this;
  }

  template <class T2> constexpr auto &operator+=(const T2 other) noexcept {
    return *this = *this + other;
  }
  template <class T2> constexpr auto &operator-=(const T2 other) noexcept {
    return *this = *this - other;
  }
  template <class T2> constexpr auto &operator*=(const T2 other) noexcept {
    return *this = *this * other;
  }
  template <class T2> constexpr auto &operator/=(const T2 other) noexcept {
    return *this = *this / other;
  }

  template <class T2>
  explicit constexpr operator ValueAndVariance<T2>() const noexcept {
    return {static_cast<T2>(value), static_cast<T2>(variance)};
  }
};

template <class T>
constexpr auto operator-(const ValueAndVariance<T> a) noexcept {
  return ValueAndVariance{-a.value, a.variance};
}

template <class T> constexpr auto sqrt(const ValueAndVariance<T> a) noexcept {
  using std::sqrt;
  return ValueAndVariance{sqrt(a.value),
                          static_cast<T>(0.25 * (a.variance / a.value))};
}

template <class T> constexpr auto abs(const ValueAndVariance<T> a) noexcept {
  using std::abs;
  return ValueAndVariance{abs(a.value), a.variance};
}

template <class T1, class T2>
constexpr auto operator+(const ValueAndVariance<T1> a,
                         const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{a.value + b.value, a.variance + b.variance};
}
template <class T1, class T2>
constexpr auto operator-(const ValueAndVariance<T1> a,
                         const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{a.value - b.value, a.variance + b.variance};
}
template <class T1, class T2>
constexpr auto operator*(const ValueAndVariance<T1> a,
                         const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{a.value * b.value,
                          a.variance * b.value * b.value +
                              b.variance * a.value * a.value};
}
template <class T1, class T2>
constexpr auto operator/(const ValueAndVariance<T1> a,
                         const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{
      a.value / b.value,
      (a.variance + b.variance * (a.value * a.value) / (b.value * b.value)) /
          (b.value * b.value)};
}

template <class T1, class T2>
constexpr auto operator+(const ValueAndVariance<T1> a, const T2 b) noexcept {
  return ValueAndVariance{a.value + b, a.variance};
}
template <class T1, class T2>
constexpr auto operator+(const T2 a, const ValueAndVariance<T1> b) noexcept {
  return ValueAndVariance{a + b.value, b.variance};
}
template <class T1, class T2>
constexpr auto operator-(const ValueAndVariance<T1> a, const T2 b) noexcept {
  return ValueAndVariance{a.value - b, a.variance};
}
template <class T1, class T2>
constexpr auto operator-(const T2 a, const ValueAndVariance<T1> b) noexcept {
  return ValueAndVariance{a - b.value, b.variance};
}
template <class T1, class T2>
constexpr auto operator*(const ValueAndVariance<T1> a, const T2 b) noexcept {
  return ValueAndVariance{a.value * b, a.variance * b * b};
}
template <class T1, class T2>
constexpr auto operator*(const T1 a, const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{a * b.value, a * a * b.variance};
}
template <class T1, class T2>
constexpr auto operator/(const ValueAndVariance<T1> a, const T2 b) noexcept {
  return ValueAndVariance{a.value / b, a.variance / (b * b)};
}
template <class T1, class T2>
constexpr auto operator/(const T1 a, const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{a / b.value, b.variance * a * a /
                                           (b.value * b.value) /
                                           (b.value * b.value)};
}

/// Deduction guide for class ValueAndVariances. Using decltype to deal with
/// potential mixed-type val and var arguments arising in binary operations
/// between, e.g., double and float.
template <class T1, class T2>
ValueAndVariance(const T1 &val, const T2 &var)
    ->ValueAndVariance<decltype(T1() + T2())>;
template <class T>
ValueAndVariance(const span<T> &val, const span<T> &var)
    ->ValueAndVariance<span<T>>;

template <class T> struct is_ValueAndVariance : std::false_type {};
template <class T>
struct is_ValueAndVariance<ValueAndVariance<T>> : std::true_type {};
template <class T>
inline constexpr bool is_ValueAndVariance_v = is_ValueAndVariance<T>::value;

} // namespace detail

} // namespace scipp::core

#endif // VALUE_AND_VARIANCE_H
