// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/structures.h"
#include "scipp/core/eigen.h"
#include "scipp/variable/structure_array_model.h"

namespace scipp::variable {

template <class T, class Elem, scipp::index N>
Variable make_structures(const Dimensions &dims, const units::Unit &unit,
                         element_array<double> &&values) {
  return {dims, std::make_shared<StructureArrayModel<T, Elem, N>>(
                    dims.volume(), unit, std::move(values))};
}

template SCIPP_VARIABLE_EXPORT Variable
make_structures<Eigen::Vector3d, double, 3>(const Dimensions &dims,
                                            const units::Unit &unit,
                                            element_array<double> &&values);

template SCIPP_VARIABLE_EXPORT Variable
make_structures<Eigen::Matrix3d, double, 9>(const Dimensions &dims,
                                            const units::Unit &unit,
                                            element_array<double> &&values);

/// Construct a variable containing vectors from a variable of elements.
Variable make_vectors(const Dimensions &dims, const units::Unit &unit,
                      element_array<double> &&values) {
  return make_structures<Eigen::Vector3d, double, 3>(dims, unit,
                                                     std::move(values));
}

/// Construct a variable containing matrices from a variable of elements.
Variable make_matrices(const Dimensions &dims, const units::Unit &unit,
                       element_array<double> &&values) {
  return make_structures<Eigen::Matrix3d, double, 9>(dims, unit,
                                                     std::move(values));
}

} // namespace scipp::variable
