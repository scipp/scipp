// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/eigen.h"
#include "scipp/variable/element_array_variable.tcc"
#include "scipp/variable/variable.h"

namespace scipp::variable {

// Variable's elements are views into other variables. Used internally for
// implementing functionality for event data combined with dense data using
// transform.
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_float64, std::span<const double>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_float32, std::span<const float>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_float64, std::span<double>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_float32, std::span<float>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_int64, std::span<const int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_int32, std::span<const int32_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_int64, std::span<int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_int32, std::span<int32_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_bool, std::span<const bool>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_bool, std::span<bool>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_datetime64,
                                   std::span<scipp::core::time_point>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_datetime64,
                                   std::span<const core::time_point>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_string,
                                   std::span<const std::string>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_string, std::span<std::string>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_vector_3_float64,
                                   std::span<const Eigen::Vector3d>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_vector_3_float64,
                                   std::span<Eigen::Vector3d>)

} // namespace scipp::variable
