// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <string>
#include <unordered_map>

#include "scipp/core/subbin_sizes.h"
#include "scipp/variable/element_array_variable.tcc"
#include "scipp/variable/variable.h"

namespace scipp::variable {

// Used internally in implementation of grouping and binning
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_float64_to_int64,
                                   std::unordered_map<double, int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_float64_to_int32,
                                   std::unordered_map<double, int32_t>)

INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_float32_to_int64,
                                   std::unordered_map<float, int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_float32_to_int32,
                                   std::unordered_map<float, int32_t>)

INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_int64_to_int64,
                                   std::unordered_map<int64_t, int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_int64_to_int32,
                                   std::unordered_map<int64_t, int32_t>)

INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_int32_to_int64,
                                   std::unordered_map<int32_t, int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_int32_to_int32,
                                   std::unordered_map<int32_t, int32_t>)

INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_bool_to_int64,
                                   std::unordered_map<bool, int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_bool_to_int32,
                                   std::unordered_map<bool, int32_t>)

INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_string_to_int64,
                                   std::unordered_map<std::string, int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(unordered_map_string_to_int32,
                                   std::unordered_map<std::string, int32_t>)

INSTANTIATE_ELEMENT_ARRAY_VARIABLE(
    unordered_map_datetime64_to_int64,
    std::unordered_map<core::time_point, int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(
    unordered_map_datetime64_to_int32,
    std::unordered_map<core::time_point, int32_t>)

INSTANTIATE_ELEMENT_ARRAY_VARIABLE(SubbinSizes, core::SubbinSizes)

} // namespace scipp::variable
