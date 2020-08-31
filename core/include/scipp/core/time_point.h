// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include "scipp/units/except.h"
#include "scipp/units/unit.h"
#include <stdint.h>
#include <string>
#define NS_LENGTH 19
#define S_LENGTH 10

namespace scipp::core {

/// Time-point similar to std::chrono::time point but without compile-time unit.
class time_point {
public:
  constexpr time_point() noexcept = default;
  time_point(const int64_t &d) { initialize(d); };
  time_point(const int32_t &d) { initialize(static_cast<int64_t>(d)); };

  void initialize(const int64_t &d) {
    m_duration = d;
    int32_t length = std::to_string(m_duration).length();
    if (length == NS_LENGTH)
      m_unit = scipp::units::ns;
    else if (length == S_LENGTH)
      m_unit = scipp::units::s;
    else
      throw except::UnitError(
          "Time point should only have time units (ns or s). " +
          std::to_string(m_duration));
  }

  scipp::units::Unit unit() const noexcept { return m_unit; };

  int64_t time_since_epoch() const noexcept { return m_duration; };

  time_point &operator+=(const int64_t duration) {
    m_duration += duration;
    return *this;
  };
  time_point &operator-=(const int64_t duration) {
    m_duration -= duration;
    return *this;
  };
  time_point operator+(const int64_t &duration) {
    return time_point(m_duration + duration);
  }
  time_point operator-(const int64_t &duration) {
    return time_point(m_duration - duration);
  }
  int64_t operator-(const time_point &time) const noexcept {
    return m_duration - time.time_since_epoch();
  }
  bool operator==(const time_point &time) const noexcept {
    return m_duration == time.time_since_epoch();
  }
  bool operator!=(const time_point &time) const noexcept {
    return m_duration != time.time_since_epoch();
  }
  bool operator<(const time_point &time) const noexcept {
    return m_duration < time.time_since_epoch();
  }
  bool operator>(const time_point &time) const noexcept {
    return m_duration > time.time_since_epoch();
  }
  bool operator<=(const time_point &time) const noexcept {
    return m_duration <= time.time_since_epoch();
  }
  bool operator>=(const time_point &time) const noexcept {
    return m_duration >= time.time_since_epoch();
  }

private:
  int64_t m_duration{0};
  scipp::units::Unit m_unit{scipp::units::dimensionless};
};

} // namespace scipp::core
