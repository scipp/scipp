// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/matrix.h"
#include "scipp/core/eigen.h"
#include "scipp/variable/matrix_model.h"

namespace scipp::variable {

/// Construct a variable containing vectors from a variable of elements.
Variable make_vectors(const Variable &elements) {
  using Model = variable::MatrixModel<Eigen::Vector3d, 3>;
  auto dims = elements.dims();
  if (dims[dims.inner()] != 3)
    throw except::DimensionError("Creating vectors of length 3 from elements "
                                 "requires inner dimension of size 3.");
  dims.erase(dims.inner());
  return Variable(dims, std::make_shared<Model>(elements));
}

/// Construct a variable containing matrices from a variable of elements.
Variable make_matrices(const Variable &elements) {
  using Model = variable::MatrixModel<Eigen::Matrix3d, 3, 3>;
  auto dims = elements.dims();
  const auto ndim = dims.ndim();
  if (ndim < 2 || dims.size(ndim - 2) != 3 || dims.size(ndim - 1) != 3)
    throw except::DimensionError("Creating matrices of size 3x3 from elements "
                                 "requires inner dimensions of sizes 3 and 3.");
  dims.erase(dims.inner());
  dims.erase(dims.inner());
  return Variable(dims, std::make_shared<Model>(elements));
}

} // namespace scipp::variable
