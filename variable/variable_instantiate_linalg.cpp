// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/eigen.h"
#include "scipp/variable/structure_array_variable.tcc"
#include "scipp/variable/variable.h"

namespace scipp::variable {

// TODO For now we use hard-coded field names and offsets. The intention is to
// generalize StructureArrayModel to support more general structures. Field
// names and sizes/offsets would then be stored as part of the model, and would
// be initialized dynamically at runtime.
template <>
constexpr auto structure_element_offset<Eigen::Vector3d> =
    [](const std::string &key) {
      static std::map<std::string, scipp::index> offsets{
          {"x", 0}, {"y", 1}, {"z", 2}};
      return offsets.at(key);
    };

template <>
constexpr auto structure_element_offset<Eigen::Matrix3d> =
    [](const std::string &key) {
      static std::map<std::string, scipp::index> offsets{
          {"xx", 0}, {"xy", 3}, {"xz", 6}, {"yx", 1}, {"yy", 4},
          {"yz", 7}, {"zx", 2}, {"zy", 5}, {"zz", 8}};
      return offsets.at(key);
    };

std::vector<std::string> element_keys(const Variable &var) {
  if (variableFactory().elem_dtype(var) == dtype<Eigen::Vector3d>)
    return {"x", "y", "z"};
  if (variableFactory().elem_dtype(var) == dtype<Eigen::Matrix3d>)
    return {"xx", "xy", "xz", "yx", "yy", "yz", "zx", "zy", "zz"};
  throw except::TypeError("dtype is not structured");
}

INSTANTIATE_STRUCTURE_ARRAY_VARIABLE(vector_3_float64, Eigen::Vector3d, double)
INSTANTIATE_STRUCTURE_ARRAY_VARIABLE(matrix_3_float64, Eigen::Matrix3d, double)

} // namespace scipp::variable
