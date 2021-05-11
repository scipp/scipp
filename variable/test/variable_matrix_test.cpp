// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/variable/matrix.h"

using namespace scipp;

class VariableMatrixTest : public ::testing::Test {
protected:
  Dimensions inner{Dim::X, 3};
  Variable elems = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                        Values{1, 2, 3, 4, 5, 6});
};

TEST_F(VariableMatrixTest, basics) {
  Variable var = variable::make_vectors(elems);
  EXPECT_EQ(var.dtype(), dtype<Eigen::Vector3d>);
  EXPECT_EQ(var.values<Eigen::Vector3d>()[0], Eigen::Vector3d(1, 2, 3));
  EXPECT_EQ(var.values<Eigen::Vector3d>()[1], Eigen::Vector3d(4, 5, 6));
}

TEST_F(VariableMatrixTest, elem_access) {
  Variable var = variable::make_vectors(elems);
  EXPECT_EQ(var.elements<Eigen::Vector3d>(scipp::index(0)),
            elems.slice({Dim::X, 0}));
  EXPECT_EQ(var.elements<Eigen::Vector3d>(scipp::index(1)),
            elems.slice({Dim::X, 1}));
  EXPECT_EQ(var.elements<Eigen::Vector3d>(scipp::index(2)),
            elems.slice({Dim::X, 2}));
}
