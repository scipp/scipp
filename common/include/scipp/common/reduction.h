// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file

#pragma once

namespace scipp::common {
template <class T, class Op>
typename T::value_type reduce_all_dims(const T &obj, const Op &op) {
  if (obj.dims().empty())
    return typename T::value_type(obj);
  typename T::value_type out = op(obj, obj.dims().inner());
  while (!out.dims().empty())
    out = op(out, out.dims().inner());
  return out;
}
} // namespace scipp::common
