// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-core_export.h"
#include "scipp/core/variable.h"

namespace scipp::core {

[[nodiscard]] SCIPP_CORE_EXPORT Variable mean(const VariableConstView &var,
                                              const Dim dim);
SCIPP_CORE_EXPORT VariableView mean(const VariableConstView &var, const Dim dim,
                                    const VariableView &out);
[[nodiscard]] SCIPP_CORE_EXPORT Variable flatten(const VariableConstView &var,
                                                 const Dim dim);
[[nodiscard]] SCIPP_CORE_EXPORT Variable sum(const VariableConstView &var,
                                             const Dim dim);
SCIPP_CORE_EXPORT VariableView sum(const VariableConstView &var, const Dim dim,
                                   const VariableView &out);

// Logical reductions
[[nodiscard]] SCIPP_CORE_EXPORT Variable any(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable any(const VariableConstView &var,
                                             const Dim dim);
[[nodiscard]] SCIPP_CORE_EXPORT Variable all(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable all(const VariableConstView &var,
                                             const Dim dim);

// Other reductions
[[nodiscard]] SCIPP_CORE_EXPORT Variable max(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable max(const VariableConstView &var,
                                             const Dim dim);
[[nodiscard]] SCIPP_CORE_EXPORT Variable min(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable min(const VariableConstView &var,
                                             const Dim dim);

} // namespace scipp::core
