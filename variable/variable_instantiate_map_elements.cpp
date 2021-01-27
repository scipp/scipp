// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <string>
#include <unordered_map>

#include "scipp/core/subbin_sizes.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

// Used internally in implementation of grouping and binning
INSTANTIATE_VARIABLE(unordered_map_double_to_int64_t,
                     std::unordered_map<double, int64_t>)
INSTANTIATE_VARIABLE(unordered_map_double_to_int32_t,
                     std::unordered_map<double, int32_t>)

INSTANTIATE_VARIABLE(unordered_map_float_to_int64_t,
                     std::unordered_map<float, int64_t>)
INSTANTIATE_VARIABLE(unordered_map_float_to_int32_t,
                     std::unordered_map<float, int32_t>)

INSTANTIATE_VARIABLE(unordered_map_float64_to_int64_t,
                     std::unordered_map<int64_t, int64_t>)
INSTANTIATE_VARIABLE(unordered_map_float64_to_int32_t,
                     std::unordered_map<int64_t, int32_t>)

INSTANTIATE_VARIABLE(unordered_map_float32_to_int64_t,
                     std::unordered_map<int32_t, int64_t>)
INSTANTIATE_VARIABLE(unordered_map_float32_to_int32_t,
                     std::unordered_map<int32_t, int32_t>)

INSTANTIATE_VARIABLE(unordered_map_bool_to_int64_t,
                     std::unordered_map<bool, int64_t>)
INSTANTIATE_VARIABLE(unordered_map_bool_to_int32_t,
                     std::unordered_map<bool, int32_t>)

INSTANTIATE_VARIABLE(unordered_map_string_to_int64_t,
                     std::unordered_map<std::string, int64_t>)
INSTANTIATE_VARIABLE(unordered_map_string_to_int32_t,
                     std::unordered_map<std::string, int32_t>)

INSTANTIATE_VARIABLE(SubbinSizes, core::SubbinSizes)

} // namespace scipp::variable
