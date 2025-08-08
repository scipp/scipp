// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "scipp/dataset/to_unit.h"
#include "scipp/units/unit.h"
#include "scipp/variable/to_unit.h"

namespace scipp::dataset {

DataArray to_unit(const DataArray &array, const sc_units::Unit &unit,
                  const CopyPolicy copy) {
  auto new_data = to_unit(array.data(), unit, copy);
  auto new_masks = new_data.is_same(array.data())
                       ? array.masks()
                       : dataset::copy(array.masks());
  return DataArray(std::move(new_data), array.coords(), std::move(new_masks),
                   array.name());
}

} // namespace scipp::dataset
