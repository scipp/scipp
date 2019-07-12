// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_COUNTS_H
#define SCIPP_CORE_COUNTS_H

#include <vector>

#include "scipp-core_export.h"
#include "scipp/units/unit.h"

namespace scipp::core {

class Dataset;
class DataProxy;
class Variable;

namespace counts {
SCIPP_CORE_EXPORT void toDensity(const DataProxy data,
                                 const std::vector<Variable> &binWidths);
SCIPP_CORE_EXPORT Dataset toDensity(Dataset d, const Dim dim);
SCIPP_CORE_EXPORT Dataset toDensity(Dataset d, const std::vector<Dim> &dims);
SCIPP_CORE_EXPORT void fromDensity(const DataProxy data,
                                   const std::vector<Variable> &binWidths);
SCIPP_CORE_EXPORT Dataset fromDensity(Dataset d, const Dim dim);
SCIPP_CORE_EXPORT Dataset fromDensity(Dataset d, const std::vector<Dim> &dims);
} // namespace counts
} // namespace scipp::core

#endif // SCIPP_CORE_COUNTS_H
