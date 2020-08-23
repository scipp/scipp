// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

/// Macro for instantiating classes and functions required for support a new
/// bucket dtype in Variable.
#define INSTANTIATE_BUCKET_VARIABLE(name, ...)                                 \
  INSTANTIATE_VARIABLE_BASE(name, __VA_ARGS__)

} // namespace scipp::variable
