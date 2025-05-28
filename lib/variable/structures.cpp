// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/structures.h"
#include "scipp/core/eigen.h"
#include "scipp/core/spatial_transforms.h"
#include "scipp/variable/structure_array_model.h"

namespace scipp::variable {

template <class T, class Elem>
Variable make_structures(const Dimensions &dims, const sc_units::Unit &unit,
                         element_array<double> &&values) {
  return {dims, std::make_shared<StructureArrayModel<T, Elem>>(
                    dims.volume(), unit, std::move(values))};
}

template SCIPP_VARIABLE_EXPORT Variable
make_structures<Eigen::Vector3d, double>(const Dimensions &,
                                         const sc_units::Unit &,
                                         element_array<double> &&);

template SCIPP_VARIABLE_EXPORT Variable
make_structures<Eigen::Matrix3d, double>(const Dimensions &,
                                         const sc_units::Unit &,
                                         element_array<double> &&);

template SCIPP_VARIABLE_EXPORT Variable
make_structures<Eigen::Affine3d, double>(const Dimensions &,
                                         const sc_units::Unit &,
                                         element_array<double> &&);

template SCIPP_VARIABLE_EXPORT Variable
make_structures<scipp::core::Quaternion, double>(const Dimensions &,
                                                 const sc_units::Unit &,
                                                 element_array<double> &&);

template SCIPP_VARIABLE_EXPORT Variable
make_structures<scipp::core::Translation, double>(const Dimensions &,
                                                  const sc_units::Unit &,
                                                  element_array<double> &&);

/// Construct a variable containing vectors from a variable of elements.
Variable make_vectors(const Dimensions &dims, const sc_units::Unit &unit,
                      element_array<double> &&values) {
  return make_structures<Eigen::Vector3d, double>(dims, unit,
                                                  std::move(values));
}

/// Construct a variable containing matrices from a variable of elements.
Variable make_matrices(const Dimensions &dims, const sc_units::Unit &unit,
                       element_array<double> &&values) {
  return make_structures<Eigen::Matrix3d, double>(dims, unit,
                                                  std::move(values));
}

/// Construct a variable containing affine transforms from a variable of
/// elements.
Variable make_affine_transforms(const Dimensions &dims,
                                const sc_units::Unit &unit,
                                element_array<double> &&values) {
  return make_structures<Eigen::Affine3d, double>(dims, unit,
                                                  std::move(values));
}

/// Construct a variable containing rotations from a variable of elements.
Variable make_rotations(const Dimensions &dims, const sc_units::Unit &unit,
                        element_array<double> &&values) {
  return make_structures<scipp::core::Quaternion, double>(dims, unit,
                                                          std::move(values));
}

/// Construct a variable containing translations from a variable of elements.
Variable make_translations(const Dimensions &dims, const sc_units::Unit &unit,
                           element_array<double> &&values) {
  return make_structures<scipp::core::Translation, double>(dims, unit,
                                                           std::move(values));
}
} // namespace scipp::variable
