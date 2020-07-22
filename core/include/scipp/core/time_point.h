// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#pragma once

namespace scipp::core {

/// Time-point similar to std::chrono::time point but without compile-time unit.
class time_point {
public:
  time_point() = default;
  explicit time_point(const int64_t) noexcept;

  int64_t time_since_epoch() const noexcept;

  time_point &operator+=(const int64_t duration);
  time_point &operator-=(const int64_t duration);

private:
  int64_t m_duration{0};
};

time_point operator+(const time_point &a, const int64_t &b);
int64_t operator-(const time_point &a, const time_point &b);

bool operator==(const time_point &a, const time_point &b);
bool operator!=(const time_point &a, const time_point &b);
bool operator<(const time_point &a, const time_point &b); // and so on

} // namespace scipp::core
