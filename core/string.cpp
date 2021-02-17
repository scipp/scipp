// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <chrono>
#include <iomanip>
#include <mutex>
#include <sstream>

#include "scipp/units/unit.h"

#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/core/slice.h"
#include "scipp/core/string.h"
#include "scipp/core/time_point.h"

namespace scipp::core {

std::ostream &operator<<(std::ostream &os, const Dimensions &dims) {
  return os << to_string(dims);
}

std::string to_string(const Dimensions &dims) {
  if (dims.empty())
    return "{}";
  std::string s = "{{";
  for (int32_t i = 0; i < scipp::size(dims.shape()); ++i)
    s += to_string(dims.labels()[i]) + ", " + std::to_string(dims.shape()[i]) +
         "}, {";
  s.resize(s.size() - 3);
  s += "}";
  return s;
}

const std::string &to_string(const std::string &s) { return s; }
std::string_view to_string(const std::string_view s) { return s; }
std::string to_string(const char *s) { return std::string(s); }

std::string to_string(const bool b) { return b ? "True" : "False"; }

std::string to_string(const DType dtype) {
  return dtypeNameRegistry().at(dtype);
}

std::string to_string(const Slice &slice) {
  std::string end = slice.end() >= 0 ? ", " + std::to_string(slice.end()) : "";
  return "Slice(" + to_string(slice.dim()) + ", " +
         std::to_string(slice.begin()) + end + ")\n";
}

std::string to_string(const scipp::index_pair &index) {
  return '(' + std::to_string(index.first) + ", " +
         std::to_string(index.second) + ')';
}

std::map<DType, std::string> &dtypeNameRegistry() {
  static std::map<DType, std::string> registry;
  return registry;
}

namespace {
template <class Ratio> constexpr int64_t num_digits() {
  static_assert(Ratio::num == 1 || Ratio::num % 10 == 0);
  static_assert(Ratio::den == 1 || Ratio::den % 10 == 0);
  static_assert(Ratio::den > Ratio::num);
  int64_t result = 0;
  for (std::size_t i = Ratio::num; i < Ratio::den; i *= 10) {
    ++result;
  }
  return result;
}

// For synchronizing access to gmtime because its return value is shared.
std::mutex gmtime_mutex;

void put_time(std::ostream &os, const std::time_t time_point) {
  std::lock_guard guard_{gmtime_mutex};
  const std::tm *tm = std::gmtime(&time_point);
  os << std::put_time(tm, "%FT%T");
}
} // namespace

std::string to_iso_date(const scipp::core::time_point &item,
                        const units::Unit &unit) {
  using Clock = std::chrono::system_clock;

  const auto print = [&](const auto duration) {
    using Period = typename decltype(duration)::period;
    std::ostringstream oss;
    // Cast to seconds to be independent of clock precision.
    // Sub-second digits are formatted manually.
    put_time(oss,
             Clock::to_time_t(Clock::time_point{
                 std::chrono::duration_cast<std::chrono::seconds>(duration)}));
    if constexpr (std::ratio_less_v<Period, std::ratio<1, 1>>) {
      oss << '.' << std::setw(num_digits<Period>()) << std::setfill('0')
          << (duration.count() % (Period::den / Period::num));
    }
    return oss.str();
  };

  if (unit == units::ns) {
    return print(std::chrono::nanoseconds{item.time_since_epoch()});
  } else if (unit == units::s) {
    return print(std::chrono::seconds{item.time_since_epoch()});
  } else if (unit == units::us) {
    return print(std::chrono::microseconds{item.time_since_epoch()});
  }
  static const auto ms = units::Unit("ms");
  if (unit == ms) {
    return print(std::chrono::milliseconds{item.time_since_epoch()});
  }
  throw except::UnitError("Cannot display time point, unsupported unit: " +
                          to_string(unit));
}
} // namespace scipp::core
