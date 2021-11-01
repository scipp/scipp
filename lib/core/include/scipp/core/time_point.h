// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace scipp::core {

/// Time-point similar to std::chrono::time point but without compile-time unit.
/// The unit is determined by the Variable the time_point is stored in.
class time_point {
public:
  time_point() = default;
  explicit time_point(int64_t duration) : m_duration{duration} {}

  [[nodiscard]] int64_t time_since_epoch() const noexcept {
    return m_duration;
  };

  friend time_point operator+(const time_point a, const int64_t b) {
    return time_point{a.time_since_epoch() + b};
  }
  friend time_point operator+(const int64_t a, const time_point b) {
    return time_point{a + b.time_since_epoch()};
  }

  friend time_point operator-(const time_point a, const int64_t b) {
    return time_point{a.time_since_epoch() - b};
  }
  friend int64_t operator-(const time_point a, const time_point b) {
    return a.time_since_epoch() - b.time_since_epoch();
  }

  time_point &operator+=(const int64_t duration) {
    m_duration += duration;
    return *this;
  };

  time_point &operator-=(const int64_t duration) {
    m_duration -= duration;
    return *this;
  };

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
  int64_t m_duration = 0;
};

} // namespace scipp::core

namespace std {
template <> struct hash<scipp::core::time_point> {
  size_t operator()(const scipp::core::time_point &tp) const noexcept {
    const auto time = tp.time_since_epoch();
    return std::hash<std::decay_t<decltype(time)>>{}(time);
  }
};
} // namespace std
