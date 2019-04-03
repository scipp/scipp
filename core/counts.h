/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef COUNTS_H
#define COUNTS_H

#include <vector>

#include "dimension.h"

namespace scipp::core {

class Dataset;
class Variable;
class VariableSlice;

namespace counts {
void toDensity(const VariableSlice var, const std::vector<Variable> &binWidths);
Dataset toDensity(Dataset d, const Dim dim);
Dataset toDensity(Dataset d, const std::vector<Dim> &dims);
void fromDensity(const VariableSlice var,
                 const std::vector<Variable> &binWidths);
Dataset fromDensity(Dataset d, const Dim dim);
Dataset fromDensity(Dataset d, const std::vector<Dim> &dims);
bool isDensity(const Variable &var);
} // namespace counts
} // namespace scipp::core

#endif // COUNTS_H
