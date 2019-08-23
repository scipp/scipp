// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_TEST_MAKE_SPARSE_H
#define SCIPP_CORE_TEST_MAKE_SPARSE_H

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

inline auto make_sparse_variable_with_variance() {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  return makeVariable<double>(
      dims, {sparse_container<double>(), sparse_container<double>()},
      {sparse_container<double>(), sparse_container<double>()});
}

inline auto make_sparse_variable() {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  return makeVariable<double>(dims);
}

inline void
set_sparse_values(Variable &var,
                  const std::vector<sparse_container<double>> &data) {
  auto vals = var.sparseValues<double>();
  for (scipp::index i = 0; i < scipp::size(data); ++i)
    vals[i] = data[i];
}

inline void
set_sparse_variances(Variable &var,
                     const std::vector<sparse_container<double>> &data) {
  auto vals = var.sparseVariances<double>();
  for (scipp::index i = 0; i < scipp::size(data); ++i)
    vals[i] = data[i];
}

#endif // SCIPP_CORE_TEST_MAKE_SPARSE_H
