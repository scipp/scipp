// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp {

template <class T>
[[nodiscard]] T extract_ranges(const Variable &indices, const T &data,
                               const Dim dim);

SCIPP_DATASET_EXPORT Variable extract(const Variable &var,
                                      const Variable &condition);
SCIPP_DATASET_EXPORT DataArray extract(const DataArray &da,
                                       const Variable &condition);
SCIPP_DATASET_EXPORT Dataset extract(const Dataset &ds,
                                     const Variable &condition);

} // namespace scipp
