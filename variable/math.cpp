// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/math.h"
#include "scipp/core/transform_common.h"
#include "scipp/variable/math.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

Variable dot(const Variable &a, const Variable &b) {
  return transform(a, b, element::dot);
}

Variable norm(const Variable &var) { return transform(var, element::norm); }

} // namespace scipp::variable
