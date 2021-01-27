// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <chrono>
#include <iomanip>
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

const std::string to_iso_date(const scipp::core::time_point &item,
                              const units::Unit &unit) {
  int64_t ts = item.time_since_epoch();
  if (unit == units::ns) {
    // cast timestamp into duration in seconds
    const std::chrono::duration<int64_t, std::nano> dur_nano(ts);
    auto dur_sec = std::chrono::duration_cast<std::chrono::seconds>(dur_nano);
    // convert to chrono::time_point
    std::chrono::system_clock::time_point tp(dur_sec);
    // convert time_point to GMT time
    auto timet = std::chrono::system_clock::to_time_t(tp);
    std::tm *tm = std::gmtime(&timet);
    // get nanoseconds
    auto ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur_nano).count() %
        1000000000;
    std::stringstream ss;
    ss << std::put_time(tm, "%FT%T.") << std::setw(9) << std::setfill('0') << ns;
    return ss.str();
  } else if (unit == units::s) {
    // cast timestamp into duration in seconds
    const std::chrono::duration<int64_t> dur_sec(ts);
    std::chrono::system_clock::time_point tp(dur_sec);
    auto timet = std::chrono::system_clock::to_time_t(tp);
    std::tm *tm = std::gmtime(&timet);
    std::stringstream ss;
    ss << std::put_time(tm, "%FT%T");
    return ss.str();
  } else
    throw except::UnitError(
        "Time point should only have time units (ns or s).");
}

} // namespace scipp::core
