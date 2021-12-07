// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <optional>
#include <string>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "scipp/units/unit.h"

#include "scipp/core/eigen.h"
#include "scipp/core/string.h"

namespace scipp::core {

template <class T>
std::string
array_to_string(const T &arr,
                const std::optional<units::Unit> &unit = std::nullopt);

template <class T>
std::string
element_to_string(const T &item,
                  const std::optional<units::Unit> &unit = std::nullopt) {
  using core::to_string;
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
  else if constexpr (std::is_same_v<T, Eigen::Affine3d>) {
    std::stringstream ss;
    for (int row = 0; row < 4; ++row) {
      ss << "[";
      for (int col = 0; col < 4; ++col) {
        ss << to_string(item(row, col)) << " ";
      }
      ss << "], ";
    }
    return ss.str();
  } else if constexpr (std::is_same_v<T, scipp::core::Quaternion>) {
    return {"(" + to_string(item.quat().w()) + "+" +
            to_string(item.quat().x()) + "i+" + to_string(item.quat().y()) +
            "j+" + to_string(item.quat().z()) + "k), "};
  } else if constexpr (std::is_same_v<T, scipp::core::Translation>)
    return element_to_string(item.vector());
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
    scipp::index n = 4;
    if (i == n && size > 2 * n) {
      s += "..., ";
      i = size - n;
    }
    s += element_to_string(arr[i], unit);
  }
  s.resize(s.size() - 2);
  s += "]";
  return s;
}

} // namespace scipp::core
