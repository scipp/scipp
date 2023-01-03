// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

template <class T> Variable concat_bins(const Variable &var, const Dim dim);

DataArray groupby_concat_bins(const DataArray &array, const Variable &edges,
                              const Variable &groups, const Dim reductionDim);

} // namespace scipp::dataset
