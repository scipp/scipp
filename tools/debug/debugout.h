#pragma once

#include <array>
#include <iostream>
#include <sstream>
#include <vector>

#include "scipp/core/string.h"

// Use namespace std to make the operators discoverable through ADL.
namespace std { // NOLINT
template <typename T, size_t N>
std::ostream &operator<<(std::ostream &os, const std::array<T, N> &a) {
  using scipp::core::to_string;
  using std::to_string;

  os << '[';
  for (size_t i = 0; i + 1 < std::size(a); ++i) {
    os << to_string(a[i]) << ", ";
  }
  if (std::size(a) > 0) {
    os << to_string(a.back());
  }
  os << ']';
  return os;
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
  using scipp::core::to_string;
  using std::to_string;

  os << '[';
  for (size_t i = 0; i + 1 < std::size(v); ++i) {
    os << to_string(v[i]) << ", ";
  }
  if (std::size(v) > 0) {
    os << to_string(v.back());
  }
  os << ']';
  return os;
}
} // namespace std