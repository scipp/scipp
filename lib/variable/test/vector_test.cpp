// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
#include <scipp/variable/vector.h>

#include <numbers>

#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/core/element/vector.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

TEST(Variable, norm_of_vector) {
  const Eigen::Vector3d v1(1, 0, -1);
  const Eigen::Vector3d v2(1, 1, 0);
  const Eigen::Vector3d v3(0, 0, -2);
  auto reference = makeVariable<double>(
      Dims{Dim::X}, Shape{3}, sc_units::m,
      Values{element::norm(v1), element::norm(v2), element::norm(v3)});
  auto var = makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                           Values{v1, v2, v3});
  EXPECT_EQ(norm(var), reference);
}

TEST(Variable, dot_of_vector) {
  const Eigen::Vector3d v1(1.1, 2.2, 3.3);
  const Eigen::Vector3d v2(-4.4, -5.5, -6.6);
  const Eigen::Vector3d v3(0, 0, 0);
  auto reference = makeVariable<double>(
      Dims{Dim::X}, Shape{3}, sc_units::m * sc_units::m,
      Values{element::dot(v1, v1), element::dot(v2, v2), element::dot(v3, v3)});
  auto var = makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                           Values{v1, v2, v3});
  const auto result = dot(var, var);
  EXPECT_TRUE(
      all(isclose(result, reference, 1e-14 * sc_units::one,
                  makeVariable<double>(Values{0.0}, sc_units::m * sc_units::m)))
          .value<bool>());
}

TEST(Variable, cross_of_vector) {
  const Eigen::Vector3d v1(1, 0, 0);
  const Eigen::Vector3d v2(0, 1, 0);
  const Eigen::Vector3d v3(0, 0, 1);

  auto reference = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{3},
      sc_units::Unit(sc_units::m) * sc_units::Unit(sc_units::m),
      Values{element::cross(v1, v2), element::cross(v2, v1),
             element::cross(v2, v2)});
  auto var1 = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{3}, sc_units::Unit(sc_units::m), Values{v1, v2, v2});
  auto var2 = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{3}, sc_units::Unit(sc_units::m), Values{v2, v1, v2});
  EXPECT_EQ(cross(var1, var2), reference);
}

TEST(Variable, angle_between_vectors) {
  const Eigen::Vector3d v1(4.7, 0, 0);
  const Eigen::Vector3d v2(2.4, -2.4, 0);

  using std::numbers::pi;
  const auto reference = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                              sc_units::Unit(sc_units::rad),
                                              Values{pi / 4, 3 * pi / 4});
  const auto var1 = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{2}, sc_units::Unit(sc_units::m), Values{v1, v1});
  const auto var2 = makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{2},
                                                  sc_units::Unit(sc_units::m),
                                                  Values{v2, (-v2).eval()});
  EXPECT_EQ(angle_between(var1, var2), reference);
}
