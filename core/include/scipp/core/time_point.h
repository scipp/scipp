// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

#include <stdint.h>
#include <string>

namespace scipp::core {

/// Time-point similar to std::chrono::time point but without compile-time unit.
class time_point {
public:
  int64_t duration{0};

  constexpr time_point() noexcept = default;
  constexpr time_point(const int64_t &d) noexcept : duration(d){};
  constexpr time_point(const int32_t &d) noexcept
      : duration(static_cast<int64_t>(d)){};

  int64_t time_since_epoch() const noexcept { return duration; };

  time_point &operator+=(const int64_t duration) {
    duration += duration;
    return *this;
  };
  time_point &operator-=(const int64_t duration) {
    duration -= duration;
    return *this;
  };
  time_point operator+(const int64_t &duration) {
    return time_point(duration + duration);
  }
  time_point operator-(const int64_t &duration) {
    return time_point(duration - duration);
  }
  int64_t operator-(const time_point &time) const noexcept {
    return duration - time.time_since_epoch();
  }
  bool operator==(const time_point &time) const noexcept {
    return duration == time.time_since_epoch();
  }
  bool operator!=(const time_point &time) const noexcept {
    return duration != time.time_since_epoch();
  }
  bool operator<(const time_point &time) const noexcept {
    return duration < time.time_since_epoch();
  }
  bool operator>(const time_point &time) const noexcept {
    return duration > time.time_since_epoch();
  }
  bool operator<=(const time_point &time) const noexcept {
    return duration <= time.time_since_epoch();
  }
  bool operator>=(const time_point &time) const noexcept {
    return duration >= time.time_since_epoch();
  }
};

} // namespace scipp::core
