// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/matrix_model.h"
#include "scipp/variable/operations.h"

using namespace scipp;

class VariableMatrixTest : public ::testing::Test {
protected:
  Dimensions inner{Dim::X, 3};
  Variable elems = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                        Values{1, 2, 3, 4, 5, 6});
};

TEST_F(VariableMatrixTest, basics) {
  Variable var(
      Dimensions(Dim::Y, 2),
      std::make_shared<variable::MatrixModel<Eigen::Vector3d, 3>>(elems));
  EXPECT_EQ(var.dtype(), dtype<Eigen::Vector3d>);
  EXPECT_EQ(var.values<Eigen::Vector3d>()[0], Eigen::Vector3d(1, 2, 3));
  EXPECT_EQ(var.values<Eigen::Vector3d>()[1], Eigen::Vector3d(4, 5, 6));
}
