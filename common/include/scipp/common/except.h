// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <initializer_list>
#include <stdexcept>
#include <string>

namespace scipp::except {

template <class T> struct Error : public std::runtime_error {
  using std::runtime_error::runtime_error;
  template <class T2>
  Error(const T2 &object, const std::string &message)
      : std::runtime_error(to_string(object) + message) {}
};

template <class Expected, class Actual>
[[noreturn]] void throw_mismatch_error(const Expected &expected,
                                       const Actual &actual,
                                       const std::string &optional_message) {
  throw Error<std::decay_t<Expected>>("Expected  " + to_string(expected) +
                                      " to be equal to " + to_string(actual) +
                                      '.' + optional_message);
}

template <class Expected, class Actual>
[[noreturn]] void
throw_mismatch_error(const Expected &expected,
                     const std::initializer_list<Actual> actual) {
  throw Error<std::decay_t<Expected>>("Expected " + to_string(expected) +
                                      " to be equal to one of " +
                                      to_string(actual));
}
} // namespace scipp::except
