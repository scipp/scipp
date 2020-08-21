// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bucket_variable.tcc"

namespace scipp::variable {

INSTANTIATE_BUCKET_VARIABLE(bucket_Variable, bucket<Variable>)

} // namespace scipp::variable
