// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

using namespace scipp;

template <typename T>
inline auto make_events_variable_with_variance(int length = 2) {
  Dimensions dims(Dim::Y, length);
  return makeVariable<event_list<T>>(
      Dimensions(dims), Values{event_list<T>(), event_list<T>()},
      Variances{event_list<T>(), event_list<T>()});
}

template <typename T> inline auto make_events_variable(int length = 2) {
  Dimensions dims(Dim::Y, length);
  return makeVariable<event_list<T>>(Dimensions(dims));
}

template <typename T>
inline void set_events_values(Variable &var,
                              const std::vector<event_list<T>> &data) {
  auto vals = var.values<event_list<T>>();
  for (scipp::index i = 0; i < scipp::size(data); ++i)
    vals[i] = data[i];
}

template <typename T>
inline void set_events_variances(Variable &var,
                                 const std::vector<event_list<T>> &data) {
  auto vals = var.variances<event_list<T>>();
  for (scipp::index i = 0; i < scipp::size(data); ++i)
    vals[i] = data[i];
}
