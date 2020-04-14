// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

template <typename T>
inline auto make_sparse_variable_with_variance(int length = 2) {
  Dimensions dims(Dim::Y, length);
  return makeVariable<sparse_container<T>>(
      Dimensions(dims), Values{sparse_container<T>(), sparse_container<T>()},
      Variances{sparse_container<T>(), sparse_container<T>()});
}

template <typename T> inline auto make_sparse_variable(int length = 2) {
  Dimensions dims(Dim::Y, length);
  return makeVariable<sparse_container<T>>(Dimensions(dims));
}

template <typename T>
inline void set_sparse_values(Variable &var,
                              const std::vector<sparse_container<T>> &data) {
  auto vals = var.values<event_list<T>>();
  for (scipp::index i = 0; i < scipp::size(data); ++i)
    vals[i] = data[i];
}

template <typename T>
inline void set_sparse_variances(Variable &var,
                                 const std::vector<sparse_container<T>> &data) {
  auto vals = var.variances<event_list<T>>();
  for (scipp::index i = 0; i < scipp::size(data); ++i)
    vals[i] = data[i];
}
