// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h" // not used directly but needed downstream for DType<Eigen::Vector3d>
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"
#include <Eigen/Geometry>

namespace scipp::core::element {
constexpr auto norm = overloaded{arg_list<Eigen::Vector3d>,
                                 [](const auto &x) { return x.norm(); },
                                 [](const sc_units::Unit &x) { return x; }};

constexpr auto dot = overloaded{
    arg_list<Eigen::Vector3d>,
    [](const auto &a, const auto &b) { return a.dot(b); },
    [](const sc_units::Unit &a, const sc_units::Unit &b) { return a * b; }};

constexpr auto cross = overloaded{
    arg_list<Eigen::Vector3d>,
    [](const auto &a, const auto &b) { return a.cross(b); },
    [](const sc_units::Unit &a, const sc_units::Unit &b) { return a * b; }};
} // namespace scipp::core::element
