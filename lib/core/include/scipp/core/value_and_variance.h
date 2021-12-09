// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <cmath>

#include "scipp/common/numeric.h"
#include "scipp/common/span.h"
#include "scipp/core/dtype.h"

namespace scipp::core {

/// A value/variance pair with operators that propagate uncertainties.
///
/// This is intended for small T such as double, float, and int. It is the
/// central implementation of uncertainty propagation in scipp, for built-in
/// operations as well as custom operations using one of the transform
/// functions. Since T is assumed to be small it is copied into the class and
/// extracted later. See also ValuesAndVariances.
template <class T> struct ValueAndVariance {
  using value_type = std::remove_cv_t<T>;

  /// This constructor is essential to prevent warnings about narrowing the
  /// types in initializer lists used below
  template <class T1, class T2>
  constexpr ValueAndVariance(T1 t1, T2 t2) noexcept : value(t1), variance(t2) {}
  constexpr ValueAndVariance(const double val) noexcept
      : value(val), variance(0.0) {}
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

template <class B, class E>
constexpr auto pow(const ValueAndVariance<B> base, const E exponent) noexcept {
  const auto pow_1 = numeric::pow(base.value, exponent - 1);
  const auto var_factor = std::abs(exponent) * pow_1;
  return ValueAndVariance{pow_1 * base.value,
                          var_factor * var_factor * base.variance};
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

template <typename T> constexpr auto exp(const ValueAndVariance<T> a) noexcept {
  using std::exp;
  const auto val = exp(a.value);
  return ValueAndVariance(val, val * val * a.variance);
}

template <typename T> constexpr auto log(const ValueAndVariance<T> a) noexcept {
  using std::log;
  return ValueAndVariance(log(a.value), a.variance / (a.value * a.value));
}

template <typename T>
constexpr auto log10(const ValueAndVariance<T> a) noexcept {
  using std::log10;
  const auto x = a.value * std::log(static_cast<T>(10.0L));
  return ValueAndVariance(log10(a.value), a.variance / (x * x));
}

template <class T> constexpr auto isnan(const ValueAndVariance<T> a) noexcept {
  using numeric::isnan;
  return isnan(a.value);
}

template <class T> constexpr auto isinf(const ValueAndVariance<T> a) noexcept {
  using numeric::isinf;
  return isinf(a.value);
}

template <class T>
constexpr auto isfinite(const ValueAndVariance<T> a) noexcept {
  using numeric::isfinite;
  return isfinite(a.value);
}

template <class T>
constexpr auto signbit(const ValueAndVariance<T> a) noexcept {
  return numeric::signbit(a.value);
}

template <class T>
constexpr auto isposinf(const ValueAndVariance<T> a) noexcept {
  return numeric::isinf(a.value) && !signbit(a);
}

template <class T>
constexpr auto isneginf(const ValueAndVariance<T> a) noexcept {
  return numeric::isinf(a.value) && signbit(a);
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

// Comparison operators. Note that all of these IGNORE VARIANCES
template <class A, class B>
constexpr auto operator==(const ValueAndVariance<A> a,
                          const ValueAndVariance<B> b) noexcept {
  return a.value == b.value;
}
template <class A, class B>
constexpr auto operator==(const ValueAndVariance<A> a, const B b) noexcept {
  return a.value == b;
}
template <class A, class B>
constexpr auto operator==(const A a, const ValueAndVariance<B> b) noexcept {
  return a == b.value;
}

template <class A, class B>
constexpr auto operator!=(const ValueAndVariance<A> a,
                          const ValueAndVariance<B> b) noexcept {
  return !(a == b);
}

template <class A, class B>
constexpr auto operator!=(const ValueAndVariance<A> a, const B b) noexcept {
  return !(a == b);
}

template <class A, class B>
constexpr auto operator!=(const A a, const ValueAndVariance<B> b) noexcept {
  return !(a == b);
}

template <class A, class B>
constexpr auto operator<(const ValueAndVariance<A> a,
                         const ValueAndVariance<B> b) noexcept {
  return a.value < b.value;
}
template <class A, class B>
constexpr auto operator<(const ValueAndVariance<A> a, const B b) noexcept {
  return a.value < b;
}
template <class A, class B>
constexpr auto operator<(const A a, const ValueAndVariance<B> b) noexcept {
  return a < b.value;
}

template <class A, class B>
constexpr auto operator<=(const ValueAndVariance<A> a,
                          const ValueAndVariance<B> b) noexcept {
  return a.value <= b.value;
}
template <class A, class B>
constexpr auto operator<=(const ValueAndVariance<A> a, const B b) noexcept {
  return a.value <= b;
}
template <class A, class B>
constexpr auto operator<=(const A a, const ValueAndVariance<B> b) noexcept {
  return a <= b.value;
}

template <class A, class B>
constexpr auto operator>(const ValueAndVariance<A> a,
                         const ValueAndVariance<B> b) noexcept {
  return a.value > b.value;
}
template <class A, class B>
constexpr auto operator>(const ValueAndVariance<A> a, const B b) noexcept {
  return a.value > b;
}
template <class A, class B>
constexpr auto operator>(const A a, const ValueAndVariance<B> b) noexcept {
  return a > b.value;
}

template <class A, class B>
constexpr auto operator>=(const ValueAndVariance<A> a,
                          const ValueAndVariance<B> b) noexcept {
  return a.value >= b.value;
}
template <class A, class B>
constexpr auto operator>=(const ValueAndVariance<A> a, const B b) noexcept {
  return a.value >= b;
}
template <class A, class B>
constexpr auto operator>=(const A a, const ValueAndVariance<B> b) noexcept {
  return a >= b.value;
}
// end comparison operators

template <class T>
constexpr auto min(const ValueAndVariance<T> a,
                   const ValueAndVariance<T> b) noexcept {
  return a.value < b.value ? a : b;
}
template <class T>
constexpr auto max(const ValueAndVariance<T> a,
                   const ValueAndVariance<T> b) noexcept {
  return a.value > b.value ? a : b;
}

/// Deduction guide for class ValueAndVariances. Using decltype to deal with
/// potential mixed-type val and var arguments arising in binary operations
/// between, e.g., double and float.
template <class T1, class T2>
ValueAndVariance(const T1 &val, const T2 &var)
    -> ValueAndVariance<decltype(T1() + T2())>;
template <class T>
ValueAndVariance(const scipp::span<T> &val, const scipp::span<T> &var)
    -> ValueAndVariance<scipp::span<T>>;

template <class T> struct is_ValueAndVariance : std::false_type {};
template <class T>
struct is_ValueAndVariance<ValueAndVariance<T>> : std::true_type {};
template <class T>
inline constexpr bool is_ValueAndVariance_v = is_ValueAndVariance<T>::value;

} // namespace scipp::core

namespace scipp {
using core::is_ValueAndVariance_v;
using core::ValueAndVariance;
} // namespace scipp
