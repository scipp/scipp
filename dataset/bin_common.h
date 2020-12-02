// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

template <class T>
Variable concat_bins(const VariableConstView &var, const Dim dim);

DataArray groupby_concat_bins(const DataArrayConstView &array,
                              const VariableConstView &edges,
                              const VariableConstView &groups,
                              const Dim reductionDim);

} // namespace scipp::dataset
