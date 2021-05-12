// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/matrix.h"
#include "scipp/core/eigen.h"
#include "scipp/variable/matrix_model.h"

namespace scipp::variable {

namespace {
template <class T, int... N>
Variable make_array_model(const Dimensions &dims, const units::Unit &unit,
                          element_array<double> &&values) {
  return {dims, std::make_shared<MatrixModel<T, 3>>(dims.volume(), unit,
                                                    std::move(values))};
}
} // namespace

/// Construct a variable containing vectors from a variable of elements.
Variable make_vectors(const Dimensions &dims, const units::Unit &unit,
                      element_array<double> &&values) {
  return make_array_model<Eigen::Vector3d, 3>(dims, unit, std::move(values));
}

/// Construct a variable containing matrices from a variable of elements.
Variable make_matrices(const Dimensions &dims, const units::Unit &unit,
                       element_array<double> &&values) {
  return make_array_model<Eigen::Matrix3d, 3, 3>(dims, unit, std::move(values));
}

} // namespace scipp::variable
