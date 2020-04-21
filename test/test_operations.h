// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <gtest/gtest.h>

// We need the decltype(auto) since we are using these operators for both
// proxies and non-proxies. The former return by reference, the latter by value.
struct plus_equals {
  template <class A, class B>
  decltype(auto) operator()(A &&a, const B &b) const {
    return a += b;
  }
};
struct plus {
  template <class A, class B> auto operator()(A &&a, B &&b) const {
    return std::forward<A>(a) + std::forward<B>(b);
  }
};
struct minus_equals {
  template <class A, class B>
  decltype(auto) operator()(A &&a, const B &b) const {
    return a -= b;
  }
};
struct minus {
  template <class A, class B> auto operator()(A &&a, B &&b) const {
    return std::forward<A>(a) - std::forward<B>(b);
  }
};
struct times_equals {
  template <class A, class B>
  decltype(auto) operator()(A &&a, const B &b) const {
    return a *= b;
  }
};
struct times {
  template <class A, class B> auto operator()(A &&a, B &&b) const {
    return std::forward<A>(a) * std::forward<B>(b);
  }
};
struct divide_equals {
  template <class A, class B>
  decltype(auto) operator()(A &&a, const B &b) const {
    return a /= b;
  }
};
struct divide {
  template <class A, class B> auto operator()(A &&a, B &&b) const {
    return std::forward<A>(a) / std::forward<B>(b);
  }
};

using Binary = ::testing::Types<plus, minus, times, divide>;
using BinaryEquals =
    ::testing::Types<plus_equals, minus_equals, times_equals, divide_equals>;
