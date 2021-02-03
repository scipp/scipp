// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/logical.h"
#include "scipp/variable/logical.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

Variable Variable::operator~() const {
  return transform(*this, core::element::logical_not);
}

} // namespace scipp::variable
