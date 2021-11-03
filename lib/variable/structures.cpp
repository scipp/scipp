// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/structures.h"
#include "scipp/core/eigen.h"
#include "scipp/variable/structure_array_model.h"

namespace scipp::variable {

template <class T, class Elem>
Variable make_structures(const Dimensions &dims, const units::Unit &unit,
                         element_array<double> &&values) {
  return {dims, std::make_shared<StructureArrayModel<T, Elem>>(
                    dims.volume(), unit, std::move(values))};
}

template SCIPP_VARIABLE_EXPORT Variable
make_structures<Eigen::Vector3d, double>(const Dimensions &,
                                         const units::Unit &,
                                         element_array<double> &&);

template SCIPP_VARIABLE_EXPORT Variable
make_structures<Eigen::Matrix3d, double>(const Dimensions &,
                                         const units::Unit &,
                                         element_array<double> &&);

/// Construct a variable containing vectors from a variable of elements.
Variable make_vectors(const Dimensions &dims, const units::Unit &unit,
                      element_array<double> &&values) {
  return make_structures<Eigen::Vector3d, double>(dims, unit,
                                                  std::move(values));
}

/// Construct a variable containing matrices from a variable of elements.
Variable make_matrices(const Dimensions &dims, const units::Unit &unit,
                       element_array<double> &&values) {
  return make_structures<Eigen::Matrix3d, double>(dims, unit,
                                                  std::move(values));
}

Variable get_column(const Variable &var, const scipp::index i) {
  const auto &model =
      dynamic_cast<const StructureArrayModel<Eigen::Matrix3d, double> &>(
          var.data());
  auto dims = var.dims();
  dims.addInner(Dim::InternalStructureComponent, 3);
  return Variable{
      dims, std::make_shared<StructureArrayModel<Eigen::Vector3d, double>>(
                model.elements())}
      .slice({Dim::InternalStructureComponent, i});
}

} // namespace scipp::variable
