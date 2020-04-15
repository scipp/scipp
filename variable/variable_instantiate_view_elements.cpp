// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/variable.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

// Variable's elements are views into other variables. Used internally for
// implementing functionality for sparse data combined with dense data using
// transform.
INSTANTIATE_VARIABLE(span<const double>)
INSTANTIATE_VARIABLE(span<const float>)
INSTANTIATE_VARIABLE(span<double>)
INSTANTIATE_VARIABLE(span<float>)
INSTANTIATE_VARIABLE(span<const int64_t>)
INSTANTIATE_VARIABLE(span<const int32_t>)
INSTANTIATE_VARIABLE(span<int64_t>)
INSTANTIATE_VARIABLE(span<int32_t>)

} // namespace scipp::variable
