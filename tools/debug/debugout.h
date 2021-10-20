#pragma once

#include <array>
#include <iostream>
#include <sstream>
#include <vector>

#include "scipp/core/string.h"

namespace debug::detail {
template <class T, class C>
void print_std_container(std::ostream &os, const C &c) {
  os << '[';
  for (size_t i = 0; i + 1 < std::size(c); ++i) {
    os << c[i] << ", ";
  }
  if (std::size(c) > 0) {
    os << c.back();
  }
  os << ']';
}
} // namespace debug::detail

// Use namespace std to make the operators discoverable through ADL.
namespace std { // NOLINT
template <typename T, size_t N>
std::ostream &operator<<(std::ostream &os, const std::array<T, N> &a) {
  debug::detail::print_std_container<T>(os, a);
  return os;
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
  debug::detail::print_std_container<T>(os, v);
  return os;
}
} // namespace std