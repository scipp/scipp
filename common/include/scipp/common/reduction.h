// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file

#pragma once

namespace scipp::common {
template <class T, class Op> auto reduce_all_dims(const T &obj, const Op &op) {
  if (obj.dims().empty())
    return copy(obj);
  auto out = op(obj, obj.dims().inner());
  while (!out.dims().empty())
    out = op(out, out.dims().inner());
  return out;
}
} // namespace scipp::common
