// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-dataset_export.h"
#include "scipp/variable/variable.h"

namespace scipp::dataset::bin_detail {

SCIPP_DATASET_EXPORT void map_to_bins(Variable &out, const Variable &var,
                                      const Variable &offsets,
                                      const Variable &indices);

} // namespace scipp::dataset::bin_detail
