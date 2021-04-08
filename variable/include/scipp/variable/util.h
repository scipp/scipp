// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/generated_util.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[maybe_unused]] SCIPP_VARIABLE_EXPORT Variable &
assign_from(Variable &var, const Variable &other);
[[maybe_unused]] SCIPP_VARIABLE_EXPORT Variable
assign_from(Variable &&var, const Variable &other);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable linspace(const Variable &start,
                                                      const Variable &stop,
                                                      const Dim dim,
                                                      const scipp::index num);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable islinspace(const Variable &var,
                                                        const Dim dim);

enum class SCIPP_VARIABLE_EXPORT SortOrder { Ascending, Descending };

[[nodiscard]] SCIPP_VARIABLE_EXPORT bool
issorted(const Variable &x, const Dim dim,
         const SortOrder order = SortOrder::Ascending);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable zip(const Variable &first,
                                                 const Variable &second);
[[nodiscard]] SCIPP_VARIABLE_EXPORT std::pair<Variable, Variable>
unzip(const Variable &var);

SCIPP_VARIABLE_EXPORT void fill(Variable &var, const Variable &value);

SCIPP_VARIABLE_EXPORT void fill_zeros(Variable &var);

} // namespace scipp::variable
