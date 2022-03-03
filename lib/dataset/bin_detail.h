// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <unordered_map>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/arithmetic.h"

namespace scipp::dataset::bin_detail {

SCIPP_DATASET_EXPORT void map_to_bins(Variable &out, const Variable &var,
                                      const Variable &offsets,
                                      const Variable &indices);

} // namespace scipp::dataset::bin_detail
