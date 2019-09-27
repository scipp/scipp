// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_SORT_H
#define SCIPP_CORE_SORT_H

#include <vector>

#include <scipp/core/dataset.h>
#include <scipp/core/variable.h>
#include <scipp/units/unit.h>

namespace scipp::core {

SCIPP_CORE_EXPORT Variable sort(const VariableConstProxy &var,
                                const VariableConstProxy &key);
SCIPP_CORE_EXPORT DataArray sort(const DataConstProxy &array,
                                 const VariableConstProxy &key);
SCIPP_CORE_EXPORT DataArray sort(const DataConstProxy &array, const Dim &key);
SCIPP_CORE_EXPORT DataArray sort(const DataConstProxy &array,
                                 const std::string &key);
SCIPP_CORE_EXPORT Dataset sort(const DatasetConstProxy &dataset,
                               const VariableConstProxy &key);
SCIPP_CORE_EXPORT Dataset sort(const DatasetConstProxy &dataset,
                               const Dim &key);
SCIPP_CORE_EXPORT Dataset sort(const DatasetConstProxy &dataset,
                               const std::string &key);

} // namespace scipp::core

#endif // SCIPP_CORE_SORT_H
