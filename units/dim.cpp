// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/units/dim.h"

namespace scipp::units {

std::unordered_map<std::string, DimId> Dim::builtin_ids{
    {Dim(Dim::X).name(), Dim::X}};
std::unordered_map<std::string, DimId> Dim::custom_ids;

std::string to_string(const Dim dim) { return dim.name(); }

} // namespace scipp::units
