// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/eigen.h"
#include "scipp/variable/structure_array_variable.tcc"
#include "scipp/variable/variable.h"

namespace scipp::variable {

template <>
constexpr auto structure_element_offset<Eigen::Vector3d> = [](scipp::index i) {
  if (i < 0 || i > 2)
    throw std::out_of_range("out of range index (" + std::to_string(i) +
                            ") for element access of vector of length 3.");
  return i;
};

template <>
constexpr auto structure_element_offset<Eigen::Matrix3d> =
    [](scipp::index i, scipp::index j) {
      if (i < 0 || i > 2 || j < 0 || j > 2)
        throw std::out_of_range("out of range indices (" + std::to_string(i) +
                                "," + std::to_string(j) +
                                ") for element access of 3x3 matrix.");
      return 3 * j + i;
    };

INSTANTIATE_STRUCTURE_ARRAY_VARIABLE(vector_3_float64, Eigen::Vector3d, double,
                                     scipp::index)
INSTANTIATE_STRUCTURE_ARRAY_VARIABLE(matrix_3_float64, Eigen::Matrix3d, double,
                                     scipp::index, scipp::index)

} // namespace scipp::variable
