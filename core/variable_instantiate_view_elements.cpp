// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/variable.h"
#include "scipp/core/variable.tcc"

namespace scipp::core {

// Variable's elements are views into other variables. Used internally for
// implementing functionality for sparse data combined with dense data using
// transform.
INSTANTIATE_VARIABLE(span<double>)
INSTANTIATE_VARIABLE(span<float>)

} // namespace scipp::core
