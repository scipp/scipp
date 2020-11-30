// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/math.h"
#include "scipp/dataset/math.h"

namespace scipp::dataset {

DataArray reciprocal(const DataArrayConstView &a) {
  return DataArray(reciprocal(a.data()), a.coords(), a.masks(), a.attrs());
}

} // namespace scipp::dataset
