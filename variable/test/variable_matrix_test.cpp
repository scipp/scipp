// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/variable/structures.h"

using namespace scipp;

class VariableMatrixTest : public ::testing::Test {
protected:
  Dimensions inner{Dim::X, 3};
  Variable vectors = variable::make_vectors(Dimensions(Dim::Y, 2), units::m,
                                            {1, 2, 3, 4, 5, 6});
  Variable matrices = variable::make_matrices(
      Dimensions(Dim::Y, 2), units::m,
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19});
};

TEST_F(VariableMatrixTest, basics) {
  EXPECT_EQ(vectors.dtype(), dtype<Eigen::Vector3d>);
  EXPECT_EQ(vectors.values<Eigen::Vector3d>()[0], Eigen::Vector3d(1, 2, 3));
  EXPECT_EQ(vectors.values<Eigen::Vector3d>()[1], Eigen::Vector3d(4, 5, 6));
}

TEST_F(VariableMatrixTest, elem_access) {
  Variable elems = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                        units::m, Values{1, 2, 3, 4, 5, 6});
  for (auto i : {0, 1, 2}) {
    EXPECT_EQ(vectors.elements<Eigen::Vector3d>().slice({Dim::Internal0, i}),
              elems.slice({Dim::X, i}));
    EXPECT_EQ(vectors.elements<Eigen::Vector3d>(scipp::index(i)),
              elems.slice({Dim::X, i}));
  }
}

TEST_F(VariableMatrixTest, matrices_elem_access) {
  // storage order is column-major
  EXPECT_EQ(
      matrices.elements<Eigen::Matrix3d>(0l, 1l),
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{4, 14}));
  EXPECT_EQ(
      matrices.elements<Eigen::Matrix3d>(1l, 0l),
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{2, 12}));
}

TEST_F(VariableMatrixTest, elem_access_unit_overwrite) {
  auto elems = vectors.elements<Eigen::Vector3d>();
  EXPECT_EQ(vectors.unit(), units::m);
  EXPECT_EQ(elems.unit(), units::m);
  vectors.setUnit(units::kg);
  EXPECT_EQ(vectors.unit(), units::kg);
  EXPECT_EQ(elems.unit(), units::kg);
  elems.setUnit(units::s);
  EXPECT_EQ(vectors.unit(), units::s);
  EXPECT_EQ(elems.unit(), units::s);
}

TEST_F(VariableMatrixTest, readonly) {
  EXPECT_FALSE(vectors.elements<Eigen::Vector3d>().is_readonly());
  EXPECT_TRUE(vectors.as_const().elements<Eigen::Vector3d>().is_readonly());
}
