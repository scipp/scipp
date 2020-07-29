// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
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

std::map<DType, std::string> &dtypeNameRegistry() {
  static std::map<DType, std::string> registry;
  return registry;
}

const std::string to_iso_date(const scipp::core::time_point &item,
                              const units::Unit &unit) {
  int64_t ts = item.time_since_epoch();

  if (unit == units::ns) {
    int64_t conv = 1000000000;
    // time representation of the timestamp
    int64_t time = ts / conv;
    auto tm = *std::gmtime(&time);
    // nanoseconds part
    std::chrono::duration<int64_t, std::nano> dur(ts);
    auto ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count() %
        conv;
    std::stringstream ss;
    ss << std::put_time(&tm, "%FT%T.") << std::setw(9) << std::setfill('0')
       << ns << std::endl;
    return ss.str();
  } else if (unit == units::s) {
    auto tm = *std::gmtime(&ts);
    std::stringstream ss;
    ss << std::put_time(&tm, "%FT%T") << std::endl;
    return ss.str();
  } else
    throw except::UnitError(
        "Time point should only have time units (ns or s).");
}

} // namespace scipp::core
