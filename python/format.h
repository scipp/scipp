// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "pybind11.h"

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
  os << '[';
  for (size_t i = 0; i + 1 < std::size(v); ++i) {
    os << v[i] << ", ";
  }
  if (std::size(v) > 0) {
    os << v.back();
  }
  os << ']';
  return os;
}

namespace detail {
template <class T> decltype(auto) format_item(T &&x) {
  return std::forward<T>(x);
}

inline std::string format_item(const scipp::units::Unit &unit) {
  return to_string(unit);
}
} // namespace detail

/// Format a string from all arguments.
template <class... Args> std::string format(Args &&... args) {
  std::ostringstream oss;
  (oss << ... << detail::format_item(std::forward<Args>(args)));
  return oss.str();
}
