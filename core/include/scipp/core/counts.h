// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_COUNTS_H
#define SCIPP_CORE_COUNTS_H

#include <vector>

#include "scipp/units/unit.h"

namespace scipp::core {

class Dataset;
class DataProxy;
class Variable;

namespace counts {
void toDensity(const DataProxy data, const std::vector<Variable> &binWidths);
Dataset toDensity(Dataset d, const Dim dim);
Dataset toDensity(Dataset d, const std::vector<Dim> &dims);
void fromDensity(const DataProxy data, const std::vector<Variable> &binWidths);
Dataset fromDensity(Dataset d, const Dim dim);
Dataset fromDensity(Dataset d, const std::vector<Dim> &dims);
} // namespace counts
} // namespace scipp::core

#endif // SCIPP_CORE_COUNTS_H
