// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
make_vectors(const Variable &elements);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
make_matrices(const Variable &elements);

} // namespace scipp::variable
