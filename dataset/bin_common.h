// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::dataset {

template <class T>
Variable concat_bins(const VariableConstView &var, const Dim erase);

} // namespace scipp::dataset
