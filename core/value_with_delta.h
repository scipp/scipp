// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef VALUE_WITH_DELTA_H
#define VALUE_WITH_DELTA_H

#include <algorithm>
#include <cmath>

namespace scipp::core {

template <class T> class ValueWithDelta {
public:
  ValueWithDelta() = default;
  ValueWithDelta(const T value, const T delta)
      : m_value(value), m_delta(delta) {}

  bool operator==(const ValueWithDelta &other) const {
    return std::abs(m_value - other.m_value) < std::max(m_delta, other.m_delta);
  }

private:
  T m_value{0.0};
  T m_delta{0.0};
};

} // namespace scipp::core

#endif // VALUE_WITH_DELTA_H
