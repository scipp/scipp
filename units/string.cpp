// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/units/string.h"
namespace scipp::units {

std::string to_string(const units::Unit &unit) { return unit.name(); }

} // namespace scipp::units