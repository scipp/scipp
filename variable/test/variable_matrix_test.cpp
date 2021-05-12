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
                                        units::m, Values{1, 2, 3, 4, 5, 6});
  Variable var = variable::make_vectors(Dimensions(Dim::Y, 2), units::m,
                                        {1, 2, 3, 4, 5, 6});
};

TEST_F(VariableMatrixTest, basics) {
  EXPECT_EQ(var.dtype(), dtype<Eigen::Vector3d>);
  EXPECT_EQ(var.values<Eigen::Vector3d>()[0], Eigen::Vector3d(1, 2, 3));
  EXPECT_EQ(var.values<Eigen::Vector3d>()[1], Eigen::Vector3d(4, 5, 6));
}

TEST_F(VariableMatrixTest, elem_access) {
  for (auto i : {0, 1, 2}) {
    EXPECT_EQ(var.elements<Eigen::Vector3d>().slice({Dim::Internal0, i}),
              elems.slice({Dim::X, i}));
    EXPECT_EQ(var.elements<Eigen::Vector3d>(scipp::index(i)),
              elems.slice({Dim::X, i}));
  }
}

TEST_F(VariableMatrixTest, elem_access_unit_overwrite) {
  auto elems = var.elements<Eigen::Vector3d>();
  EXPECT_EQ(var.unit(), units::m);
  EXPECT_EQ(elems.unit(), units::m);
  var.setUnit(units::kg);
  EXPECT_EQ(var.unit(), units::kg);
  EXPECT_EQ(elems.unit(), units::kg);
  elems.setUnit(units::s);
  EXPECT_EQ(var.unit(), units::s);
  EXPECT_EQ(elems.unit(), units::s);
}
