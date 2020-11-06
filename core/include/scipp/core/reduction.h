// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/units/dim.h"

#pragma once

namespace scipp::core {
template <class T, class Op>
typename T::value_type reduce_all_dims(const T &obj, const Op &op) {
  if (obj.dims().empty())
    return typename T::value_type(obj);
  typename T::value_type out = op(obj, obj.dims().inner());
  while (!out.dims().empty())
    out = op(out, out.dims().inner());
  return out;
}
}
