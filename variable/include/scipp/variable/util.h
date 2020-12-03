// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
linspace(const VariableConstView &start, const VariableConstView &stop,
         const Dim dim, const scipp::index num);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
is_linspace(const VariableConstView &var, const Dim dim);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable values(const VariableConstView &x);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
variances(const VariableConstView &x);

enum class SCIPP_VARIABLE_EXPORT SortOrder { Ascending, Descending };

[[nodiscard]] SCIPP_VARIABLE_EXPORT bool
is_sorted(const VariableConstView &x, const Dim dim,
          const SortOrder order = SortOrder::Ascending);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
zip(const VariableConstView &first, const VariableConstView &second);
[[nodiscard]] SCIPP_VARIABLE_EXPORT std::pair<Variable, Variable>
unzip(const VariableConstView &var);

SCIPP_VARIABLE_EXPORT void fill(const VariableView &var,
                                const VariableConstView &value);

SCIPP_VARIABLE_EXPORT void fill_zeros(const VariableView &var);

} // namespace scipp::variable
