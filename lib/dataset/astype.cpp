// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "scipp/dataset/astype.h"

#include "scipp/variable/astype.h"

namespace scipp::dataset {
DataArray astype(const DataArray &array, const DType type,
                 const CopyPolicy copy) {
  auto new_data = astype(array.data(), type, copy);
  auto new_masks = new_data.is_same(array.data())
                       ? array.masks()
                       : dataset::copy(array.masks());
  return DataArray(std::move(new_data), array.coords(), std::move(new_masks),
                   array.name());
}
} // namespace scipp::dataset
