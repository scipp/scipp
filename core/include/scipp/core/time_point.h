// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <stdint.h>
#include <string>

namespace scipp::core {

/// Time-point similar to std::chrono::time point but without compile-time unit.
/// The unit is determined by the Variable the time_point is stored in.
class time_point {
public:
  time_point() = default;
  explicit time_point(int64_t duration) : m_duration{duration} {}
  time_point(const time_point &other) = default;
  time_point(time_point &&other) noexcept = default;
  time_point &operator=(const time_point &other) = default;
  time_point &operator=(time_point &&other) noexcept = default;
  ~time_point() noexcept = default;

  int64_t time_since_epoch() const noexcept { return m_duration; };

  time_point operator+(const int64_t &d) { return time_point{m_duration + d}; }
  time_point operator-(const int64_t &d) { return time_point{m_duration - d}; }
  int64_t operator-(const time_point &time) const noexcept {
    return m_duration - time.time_since_epoch();
  }

  time_point &operator+=(const int64_t duration) {
    m_duration += duration;
    return *this;
  };
  time_point &operator-=(const int64_t duration) {
    m_duration -= duration;
    return *this;
  };

  time_point &operator*=(const int64_t other) noexcept {
    m_duration *= other;
    return *this;
  };
  time_point &operator/=(const int64_t other) noexcept {
    m_duration /= other;
    return *this;
  };

  friend time_point operator*(const time_point a, const int64_t b) noexcept {
    return time_point{a.m_duration * b};
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
  int64_t m_duration;
};

} // namespace scipp::core
