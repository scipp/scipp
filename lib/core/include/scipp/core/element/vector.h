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

constexpr auto angle_between =
    overloaded{arg_list<Eigen::Vector3d>,
               [](const auto &a, const auto &b) {
                 // This implementation is based on paragraph 13 of
                 // https://people.eecs.berkeley.edu/~wkahan/MathH110/Cross.pdf
                 // By W. Kahan

                 // Trigger early computation because each value is used twice
                 // below.
                 const Eigen::Vector3d a_normed = a / a.norm();
                 const Eigen::Vector3d b_normed = b / b.norm();

                 const auto x = (a_normed + b_normed).norm();
                 const auto y = (a_normed - b_normed).norm();
                 return 2 * atan2(y, x);
               },
               [](const sc_units::Unit &, const sc_units::Unit &) {
                 // It does not make much sense to compute the angle between
                 // vectors with different units. But since all vectors are
                 // normalized, units do not impact the algorithm.
                 return sc_units::rad;
               }};
} // namespace scipp::core::element
