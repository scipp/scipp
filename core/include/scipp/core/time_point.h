// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <stdint.h>

namespace scipp::core {

/// Time-point similar to std::chrono::time point but without compile-time unit.
class time_point {
public:
  constexpr time_point() noexcept : m_duration(0){};
  constexpr time_point(const int64_t &d) noexcept : m_duration(d){};
  constexpr time_point(const int32_t &d) noexcept : m_duration(int64_t(d)){};

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
    m_duration += duration;
    return *this;
  }
  int64_t operator-(const time_point &time) const noexcept {
    int64_t ret = m_duration - time.time_since_epoch();
    return ret;
  }
  int64_t operator-(const int64_t &duration) const noexcept {
    int64_t ret = m_duration - duration;
    return ret;
  }
  bool operator==(const time_point &time) const noexcept {
    const bool ret = m_duration == time.time_since_epoch();
    return ret;
  }
  bool operator!=(const time_point &time) const noexcept {
    const bool ret = !(m_duration == time.time_since_epoch());
    return ret;
  }
  bool operator<(const time_point &time) const noexcept {
    const bool ret = m_duration < time.time_since_epoch();
    return ret;
  }
  bool operator>(const time_point &time) const noexcept {
    const bool ret = m_duration > time.time_since_epoch();
    return ret;
  }
  bool operator<=(const time_point &time) const noexcept {
    const bool ret = m_duration <= time.time_since_epoch();
    return ret;
  }
  bool operator>=(const time_point &time) const noexcept {
    const bool ret = m_duration >= time.time_since_epoch();
    return ret;
  }

private:
  int64_t m_duration{0};
};

} // namespace scipp::core
