// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "scipp/dataset/to_unit.h"
#include "scipp/units/unit.h"
#include "scipp/variable/to_unit.h"

namespace scipp::dataset {

DataArray to_unit(const DataArray &array, const units::Unit &unit,
                  const CopyPolicy copy) {
  return DataArray(to_unit(array.data(), unit, copy), array.coords(),
                   array.masks(), array.attrs(), array.name());
}

} // namespace scipp::dataset
