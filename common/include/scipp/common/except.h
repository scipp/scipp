// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <initializer_list>
#include <stdexcept>

namespace scipp::except {

template <class T> struct Error : public std::runtime_error {
  using std::runtime_error::runtime_error;
  template <class T2>
  Error(const T2 &object, const std::string &message)
      : std::runtime_error(to_string(object) + message) {}
};

template <class T> struct MismatchError : public Error<T> {
  template <class A, class B>
  MismatchError(const A &a, const B &b)
      : Error<T>(a, " expected to be equal to " + to_string(b)) {}

  template <class A, class B>
  MismatchError(const A &a, const std::initializer_list<B> b)
      : Error<T>(a, " expected to be equal to one of " + to_string(b)) {}
};

} // namespace scipp::except
