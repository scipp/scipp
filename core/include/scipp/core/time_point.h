// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <stdint.h>

namespace scipp::core {

/// Time-point similar to std::chrono::time point but without compile-time unit.
class time_point {
public:
  constexpr time_point() noexcept = default;
  constexpr time_point(const int64_t &d) noexcept : m_duration(d){};
  constexpr time_point(const int32_t &d) noexcept
      : m_duration(static_cast<int64_t>(d)){};

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
};

} // namespace scipp::core
