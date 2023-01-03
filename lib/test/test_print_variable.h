// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <ostream>
#include <vector>

#include "scipp/common/index.h"
#include "scipp/variable/variable_keyword_arg_constructor.h"

namespace scipp {
namespace testing {
template <class T>
inline void print(const std::vector<T> &vec, std::ostream &os) {
  os << '[';
  for (scipp::index i = 0; i < scipp::size(vec) - 1; ++i) {
    os << vec.at(static_cast<size_t>(i)) << ", ";
  }
  if (scipp::size(vec) > 0) {
    os << vec.back();
  }
  os << ']';
}
} // namespace testing

namespace variable::detail {
inline void PrintTo(const Shape &shape, std::ostream *os) {
  testing::print(shape.data, *os);
}
} // namespace variable::detail
} // namespace scipp
