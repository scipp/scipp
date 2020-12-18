// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <map>
#include <optional>
#include <string>

#include <Eigen/Dense>

#include "scipp/units/unit.h"

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/dtype.h"
#include "scipp/core/slice.h"

namespace scipp::core {

template <class Id, class Key, class Value> class ConstView;
template <class T, class U> class MutableView;

SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const Dimensions &dims);

SCIPP_CORE_EXPORT const std::string &to_string(const std::string &s);
SCIPP_CORE_EXPORT std::string_view to_string(const std::string_view s);
SCIPP_CORE_EXPORT std::string to_string(const char *s);
SCIPP_CORE_EXPORT std::string to_string(const bool b);
SCIPP_CORE_EXPORT std::string to_string(const DType dtype);
SCIPP_CORE_EXPORT std::string to_string(const Dimensions &dims);
SCIPP_CORE_EXPORT std::string to_string(const Slice &slice);

template <class Id, class Key, class Value>
std::string to_string(const ConstView<Id, Key, Value> &view) {
  std::stringstream ss;

  for (const auto &[key, item] : view) {
    ss << "<scipp.ConstView> (" << key << "):\n" << to_string(item);
  }
  return ss.str();
}

template <class T, class U>
std::string to_string(const MutableView<T, U> &mutableView) {
  std::stringstream ss;

  for (const auto &[key, item] : mutableView) {
    ss << "<scipp.MutableView> (" << key << "):\n" << to_string(item);
  }
  return ss.str();
}

template <class T>
std::string
array_to_string(const T &arr,
                const std::optional<units::Unit> &unit = std::nullopt);

SCIPP_CORE_EXPORT const std::string
to_iso_date(const scipp::core::time_point &item, const units::Unit &unit);

template <class T>
std::string
element_to_string(const T &item,
                  const std::optional<units::Unit> &unit = std::nullopt) {
  using std::to_string;
  if constexpr (std::is_same_v<T, std::string>)
    return {'"' + item + "\", "};
  else if constexpr (std::is_same_v<T, bool>)
    return core::to_string(item) + ", ";
  else if constexpr (std::is_same_v<T, scipp::core::time_point>) {
    return core::to_string(to_iso_date(item, unit.value())) + ", ";
  } else if constexpr (std::is_same_v<T, Eigen::Vector3d>)
    return {"(" + to_string(item[0]) + ", " + to_string(item[1]) + ", " +
            to_string(item[2]) + "), "};
  else if constexpr (std::is_same_v<T, Eigen::Matrix3d>)
    return {"(" + element_to_string(Eigen::Vector3d(item.row(0))) + ", " +
            element_to_string(Eigen::Vector3d(item.row(1))) + ", " +
            element_to_string(Eigen::Vector3d(item.row(2))) + "), "};
  else if constexpr (std::is_same_v<T, std::pair<scipp::index, scipp::index>>)
    return {"<" + to_string(item.first) + ", " + to_string(item.second) +
            ">, "};
  else
    return to_string(item) + ", ";
}

template <class T>
std::string array_to_string(const T &arr,
                            const std::optional<units::Unit> &unit) {
  const auto size = scipp::size(arr);
  if (size == 0)
    return std::string("[]");
  std::string s = "[";
  for (scipp::index i = 0; i < scipp::size(arr); ++i) {
    if (i == 2 && size > 6) {
      s += "..., ";
      i = size - 4;
    }
    s += element_to_string(arr[i], unit);
  }
  s.resize(s.size() - 2);
  s += "]";
  return s;
}

/// Return the global dtype name registry instance
SCIPP_CORE_EXPORT std::map<DType, std::string> &dtypeNameRegistry();

} // namespace scipp::core
