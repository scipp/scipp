// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/flags.h"

#include "scipp-variable_export.h"
#include "scipp/variable/generated_util.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable linspace(const Variable &start,
                                                      const Variable &stop,
                                                      const Dim dim,
                                                      const scipp::index num);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable islinspace(const Variable &var,
                                                        const Dim dim);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
issorted(const Variable &x, const Dim dim,
         const SortOrder order = SortOrder::Ascending);

[[nodiscard]] SCIPP_VARIABLE_EXPORT bool
allsorted(const Variable &x, const Dim dim,
          const SortOrder order = SortOrder::Ascending);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable zip(const Variable &first,
                                                 const Variable &second);

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::pair<Variable, Variable>
unzip(const Variable &var);

SCIPP_VARIABLE_EXPORT void fill(Variable &var, const Variable &value);

SCIPP_VARIABLE_EXPORT void fill_zeros(Variable &var);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable where(const Variable &condition,
                                                   const Variable &x,
                                                   const Variable &y);

} // namespace scipp::variable
