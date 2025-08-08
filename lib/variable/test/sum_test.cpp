// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/string.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

class SumTest : public ::testing::Test {
protected:
  Variable var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                      sc_units::m, Values{1.0, 2.0, 3.0, 4.0});
  Variable var_bool =
      makeVariable<bool>(Dims{Dim::Y, Dim::X}, Shape{2, 2}, sc_units::m,
                         Values{true, false, true, true});
};

TEST_F(SumTest, sum) {
  const auto expectedX = makeVariable<double>(Dims{Dim::Y}, Shape{2},
                                              sc_units::m, Values{3.0, 7.0});
  const auto expectedY = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                              sc_units::m, Values{4.0, 6.0});
  EXPECT_EQ(sum(var, Dim::X), expectedX);
  EXPECT_EQ(sum(var, Dim::Y), expectedY);
}

TEST_F(SumTest, sum_with_empty_dim) {
  const auto empty_slice = var.slice({Dim::X, 0, 0});
  EXPECT_EQ(
      sum(empty_slice, Dim::X),
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, sc_units::m, Values{0, 0}));
  EXPECT_EQ(
      sum(empty_slice, Dim::Y),
      makeVariable<double>(Dims{Dim::X}, Shape{0}, sc_units::m, Values{}));
}

TEST(VectorReduceTest, sum_vector) {
  const auto vector_var = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{2}, sc_units::m,
      Values{Eigen::Vector3d{1, 2, 3}, Eigen::Vector3d{4, 5, 6}});
  const auto expected = makeVariable<Eigen::Vector3d>(
      Dims{}, Shape{1}, sc_units::m, Values{Eigen::Vector3d{5, 7, 9}});
  auto summed = sum(vector_var, Dim::X);
  EXPECT_EQ(summed, expected);
}

TEST(VectorReduceTest, mean_vector) {
  const auto vector_var = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{2}, sc_units::m,
      Values{Eigen::Vector3d{1, 2, 3}, Eigen::Vector3d{4, 5, 6}});
  const auto expected = makeVariable<Eigen::Vector3d>(
      Dims{}, Shape{1}, sc_units::m, Values{Eigen::Vector3d{2.5, 3.5, 4.5}});
  auto averaged = mean(vector_var, Dim::X);
  EXPECT_EQ(averaged, expected);
}

TEST(SumPrecisionTest, sum_float) {
  const float init = 100000000.0;
  scipp::index N = 100;
  Variable var = broadcast(makeVariable<float>(Values{1.0}), {{Dim::X}, {N}});
  var = concat(std::vector{makeVariable<float>(Values{init}), var}, Dim::X);
  EXPECT_EQ(sum(var, Dim::X), makeVariable<float>(Values{init + N * 1.0}));
  for (scipp::index i = 1; i < N; i += 2)
    var.values<float>()[i] = NAN;
  EXPECT_EQ(nansum(var, Dim::X),
            makeVariable<float>(Values{init + (N / 2) * 1.0}));
}
