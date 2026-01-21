// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <chrono>
#include <format>

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

std::ostream &operator<<(std::ostream &os, const scipp::index_pair &index) {
  return os << to_string(index);
}

std::string to_string(const Dimensions &dims) {
  if (dims.empty())
    return "()";
  std::string s = "(";
  for (int32_t i = 0; i < scipp::size(dims.shape()); ++i)
    s += to_string(dims.labels()[i]) + ": " + std::to_string(dims.shape()[i]) +
         ", ";
  s.resize(s.size() - 2);
  s += ")";
  return s;
}

std::string labels_to_string(const Dimensions &dims) {
  if (dims.empty())
    return "()";
  std::string s = "(";
  for (const auto &dim : dims.labels())
    s += to_string(dim) + ", ";
  s.resize(s.size() - 2);
  s += ")";
  return s;
}

std::string to_string(const Sizes &sizes) {
  std::string repr("Sizes[");
  for (const auto &dim : sizes)
    repr += to_string(dim) + ":" + std::to_string(sizes[dim]) + ", ";
  repr += "]";
  return repr;
}

const std::string &to_string(const std::string &s) { return s; }
std::string_view to_string(const std::string_view &s) { return s; }
std::string to_string(const char *s) { return std::string(s); }

std::string to_string(const bool b) { return b ? "True" : "False"; }

std::string to_string(const DType dtype) {
  return dtypeNameRegistry().at(dtype);
}

std::string to_string(const Slice &slice) {
  std::string end = slice.end() >= 0 ? ", " + std::to_string(slice.end()) : "";
  return "Slice(" + to_string(slice.dim()) + ", " +
         std::to_string(slice.begin()) + end + ')';
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
template <class Rep, class Period>
std::string to_string(const std::chrono::duration<Rep, Period> &since_epoch) {
  using Clock = std::chrono::system_clock;
  using Duration = std::chrono::duration<Rep, Period>;

#ifdef _WIN32
  // Windows' time functions (e.g. gmtime) don't support datetimes before 1970.
  if (since_epoch < std::chrono::duration<Rep, Period>::zero()) {
    return "(datetime before 1970, cannot format)";
  }
#endif

  if constexpr (std::ratio_less_equal_v<Period, std::chrono::seconds::period>) {
    // Cast to as narrow a type as possible to get the
    // correct number of subsecond digits.
    // (%S always formats as many digits as the time point allows.)
    const auto point =
        std::chrono::time_point_cast<Duration>(Clock::time_point{since_epoch});
    return std::format("{:%Y-%m-%dT%H:%M:%S}", point);
  } else {
    // std::format does not support time points with large units, so use the
    // lowest possible unit here and use concrete format strings.
    const auto point = std::chrono::time_point_cast<std::chrono::minutes>(
        Clock::time_point{since_epoch});

    if constexpr (std::is_same_v<Duration, std::chrono::minutes>) {
      return std::format("{:%Y-%m-%dT%H:%M}", point);
    } else if constexpr (std::is_same_v<Duration, std::chrono::hours>) {
      return std::format("{:%Y-%m-%dT%H}", point);
    } else if constexpr (std::is_same_v<Duration, std::chrono::days>) {
      return std::format("{:%Y-%m-%d}", point);
    } else if constexpr (std::is_same_v<Duration, std::chrono::months>) {
      return std::format("{:%Y-%m}", point);
    } else /* years */ {
      return std::format("{:%Y}", point);
    }
  }
}
} // namespace

std::string to_iso_date(const scipp::core::time_point &item,
                        const sc_units::Unit &unit) {
  if (unit == sc_units::ns) {
    return to_string(std::chrono::nanoseconds{item.time_since_epoch()});
  } else if (unit == sc_units::s) {
    return to_string(std::chrono::seconds{item.time_since_epoch()});
  } else if (unit == sc_units::us) {
    return to_string(std::chrono::microseconds{item.time_since_epoch()});
  } else if (unit == sc_units::Unit(units::precise::ms)) {
    return to_string(std::chrono::milliseconds{item.time_since_epoch()});
  } else if (unit == sc_units::Unit(units::precise::minute)) {
    return to_string(std::chrono::minutes{item.time_since_epoch()});
  } else if (unit == sc_units::Unit(units::precise::hr)) {
    return to_string(std::chrono::hours{item.time_since_epoch()});
  } else if (unit == sc_units::Unit(units::precise::day)) {
    return to_string(std::chrono::days{item.time_since_epoch()});
  } else if (unit == sc_units::Unit("month")) {
    return to_string(std::chrono::months{item.time_since_epoch()});
  } else if (unit == sc_units::Unit("year")) {
    return to_string(std::chrono::years{item.time_since_epoch()});
  }
  throw except::UnitError("Cannot display time point, unsupported unit: " +
                          to_string(unit));
}
} // namespace scipp::core
