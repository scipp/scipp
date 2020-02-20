// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_SORT_H
#define SCIPP_CORE_SORT_H

#include <vector>

#include <scipp/core/dataset.h>
#include <scipp/core/variable.h>

namespace scipp::core {

SCIPP_CORE_EXPORT Variable sort(const VariableConstView &var,
                                const VariableConstView &key);
SCIPP_CORE_EXPORT DataArray sort(const DataArrayConstView &array,
                                 const VariableConstView &key);
SCIPP_CORE_EXPORT DataArray sort(const DataArrayConstView &array,
                                 const Dim &key);
SCIPP_CORE_EXPORT DataArray sort(const DataArrayConstView &array,
                                 const std::string &key);
SCIPP_CORE_EXPORT Dataset sort(const DatasetConstView &dataset,
                               const VariableConstView &key);
SCIPP_CORE_EXPORT Dataset sort(const DatasetConstView &dataset, const Dim &key);
SCIPP_CORE_EXPORT Dataset sort(const DatasetConstView &dataset,
                               const std::string &key);

} // namespace scipp::core

#endif // SCIPP_CORE_SORT_H
