// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset::buckets {

[[nodiscard]] SCIPP_DATASET_EXPORT Variable
concatenate(const VariableConstView &var0, const VariableConstView &var1);

} // namespace scipp::dataset::buckets
