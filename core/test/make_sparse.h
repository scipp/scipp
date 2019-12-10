// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_TEST_MAKE_SPARSE_H
#define SCIPP_CORE_TEST_MAKE_SPARSE_H

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

template <typename T>
inline auto make_sparse_variable_with_variance(int length = 2) {
  Dimensions dims({Dim::Y, Dim::X}, {length, Dimensions::Sparse});
  return makeVariable<T>(
      Dimensions(dims), Values{sparse_container<T>(), sparse_container<T>()},
      Variances{sparse_container<T>(), sparse_container<T>()});
}

template <typename T> inline auto make_sparse_variable(int length = 2) {
  Dimensions dims({Dim::Y, Dim::X}, {length, Dimensions::Sparse});
  return makeVariable<T>(Dimensions(dims));
}

template <typename T>
inline void set_sparse_values(Variable &var,
                              const std::vector<sparse_container<T>> &data) {
  auto vals = var.sparseValues<T>();
  for (scipp::index i = 0; i < scipp::size(data); ++i)
    vals[i] = data[i];
}

template <typename T>
inline void set_sparse_variances(Variable &var,
                                 const std::vector<sparse_container<T>> &data) {
  auto vals = var.sparseVariances<T>();
  for (scipp::index i = 0; i < scipp::size(data); ++i)
    vals[i] = data[i];
}

#endif // SCIPP_CORE_TEST_MAKE_SPARSE_H
