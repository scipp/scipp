// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <string>
#include <unordered_map>

#include "scipp/variable/variable.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

// Used internally in implementation of grouping and binning
INSTANTIATE_VARIABLE(unordered_map_float64_to_index,
                     std::unordered_map<int64_t, scipp::index>)
INSTANTIATE_VARIABLE(unordered_map_float32_to_index,
                     std::unordered_map<int32_t, scipp::index>)
INSTANTIATE_VARIABLE(unordered_map_string_to_index,
                     std::unordered_map<std::string, scipp::index>)

} // namespace scipp::variable
