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
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_float64,
                                   scipp::span<const double>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_float32, scipp::span<const float>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_float64, scipp::span<double>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_float32, scipp::span<float>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_int64, scipp::span<const int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_int32, scipp::span<const int32_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_int64, scipp::span<int64_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_int32, scipp::span<int32_t>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_bool, scipp::span<const bool>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_bool, scipp::span<bool>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_datetime64,
                                   scipp::span<scipp::core::time_point>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_datetime64,
                                   scipp::span<const core::time_point>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_string,
                                   scipp::span<const std::string>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_string, scipp::span<std::string>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_const_vector_3_float64,
                                   scipp::span<const Eigen::Vector3d>)
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(span_vector_3_float64,
                                   scipp::span<Eigen::Vector3d>)

} // namespace scipp::variable
