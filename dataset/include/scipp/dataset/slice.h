// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray slice(const DataArray &data,
                                                   const Dim dim,
                                                   const Variable &value);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray slice(const DataArray &data,
                                                   const Dim dim,
                                                   const Variable &begin,
                                                   const Variable &end);

[[nodiscard]] SCIPP_DATASET_EXPORT Dataset slice(const Dataset &ds,
                                                 const Dim dim,
                                                 const Variable &value);

[[nodiscard]] SCIPP_DATASET_EXPORT Dataset slice(const Dataset &ds,
                                                 const Dim dim,
                                                 const Variable &begin,
                                                 const Variable &end);

} // namespace scipp::dataset
