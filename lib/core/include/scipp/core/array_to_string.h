// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
                const std::optional<sc_units::Unit> &unit = std::nullopt);

template <class T>
std::string
element_to_string(const T &item,
                  const std::optional<sc_units::Unit> &unit = std::nullopt) {
  using core::to_string;
  using std::to_string;
  if constexpr (std::is_same_v<T, std::string>) {
    if (item.length() > 80) {
      return {'"' + item.substr(0, 77) + "...\", "};
    }
    return {'"' + item + "\", "};
  } else if constexpr (std::is_same_v<T, bool>)
    return core::to_string(item) + ", ";
  else if constexpr (std::is_same_v<T, scipp::core::time_point>) {
    return core::to_string(to_iso_date(item, unit.value())) + ", ";
  } else if constexpr (std::is_same_v<T, Eigen::Vector3d>) {
    std::stringstream ss;
    ss << "(" << item[0] << ", " << item[1] << ", " << item[2] << "), ";
    return ss.str();
  } else if constexpr (std::is_same_v<T, Eigen::Matrix3d>)
    return {"(" + element_to_string(Eigen::Vector3d(item.row(0))) + ", " +
            element_to_string(Eigen::Vector3d(item.row(1))) + ", " +
            element_to_string(Eigen::Vector3d(item.row(2))) + "), "};
  else if constexpr (std::is_same_v<T, Eigen::Affine3d>) {
    return element_to_string(item.matrix());
  } else if constexpr (std::is_same_v<T, scipp::core::Quaternion>) {
    std::stringstream ss;
    ss << '(' << item.quat().w();
    ss.setf(std::ios::showpos);
    ss << item.quat().x() << 'i' << item.quat().y() << 'j' << item.quat().z()
       << "k), ";
    return ss.str();
  } else if constexpr (std::is_same_v<T, scipp::core::Translation>)
    return element_to_string(item.vector());
  else {
    std::stringstream ss;
    ss << item << ", ";
    return ss.str();
  }
}

template <class T>
std::string scalar_array_to_string(const T &arr,
                                   const std::optional<sc_units::Unit> &unit) {
  auto s = element_to_string(arr[0], unit);
  return s.substr(0, s.size() - 2);
}

template <class T>
std::string array_to_string(const T &arr,
                            const std::optional<sc_units::Unit> &unit) {
  const auto size = scipp::size(arr);
  if (size == 0)
    return "[]";
  std::string s = "[";
  for (scipp::index i = 0; i < size; ++i) {
    constexpr scipp::index n = 2;
    if (i == n && size > 2 * n) {
      s += "..., ";
      i = size - n;
    }
    s += element_to_string(arr[i], unit);
  }
  s.resize(s.size() > 1 ? s.size() - 2 : 1);
  s += "]";
  return s;
}

} // namespace scipp::core
