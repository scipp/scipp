// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <initializer_list>
#include <stdexcept>

#include "traits.h"

namespace scipp::except {

template <class T> struct Error : public std::runtime_error {
  using std::runtime_error::runtime_error;
  template <class T2>
  Error(const T2 &object, const std::string &message)
      : std::runtime_error(to_string(object) + message) {}
};

template <class Expected, class Actual>
[[noreturn]] void throw_mismatch_error(const Expected &expected,
                                       const Actual &actual) {
  if constexpr (common::has_const_view_type_v<Expected> &&
                !common::is_const_view_type_v<Expected>) {
    throw_mismatch_error(typename Expected::const_view_type{expected}, actual);
  } else if constexpr (common::has_const_view_type_v<Actual> &&
                       !common::is_const_view_type_v<Actual>) {
    throw_mismatch_error(expected, typename Actual::const_view_type{actual});
  } else {
    throw Error<std::decay_t<Expected>>("Expected  " + to_string(expected) +
                                        " to be equal to " + to_string(actual) +
                                        '.');
  }
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
