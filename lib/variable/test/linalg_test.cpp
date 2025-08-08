// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/variable/structures.h"

#include "test_macros.h"

using namespace scipp;

class LinalgVectorTest : public ::testing::Test {
protected:
  Variable vectors = variable::make_vectors(Dimensions(Dim::Y, 2), sc_units::m,
                                            {1, 2, 3, 4, 5, 6});
};

TEST_F(LinalgVectorTest, basics) {
  EXPECT_EQ(vectors.dtype(), dtype<Eigen::Vector3d>);
  EXPECT_EQ(vectors.values<Eigen::Vector3d>()[0], Eigen::Vector3d(1, 2, 3));
  EXPECT_EQ(vectors.values<Eigen::Vector3d>()[1], Eigen::Vector3d(4, 5, 6));
}

TEST_F(LinalgVectorTest, elem_access) {
  Variable elems = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                        sc_units::m, Values{1, 2, 3, 4, 5, 6});
  for (auto i : {0, 1, 2}) {
    EXPECT_EQ(vectors.elements<Eigen::Vector3d>().slice(
                  {Dim::InternalStructureComponent, i}),
              elems.slice({Dim::X, i}));
  }
  EXPECT_EQ(vectors.elements<Eigen::Vector3d>("x"), elems.slice({Dim::X, 0}));
  EXPECT_EQ(vectors.elements<Eigen::Vector3d>("y"), elems.slice({Dim::X, 1}));
  EXPECT_EQ(vectors.elements<Eigen::Vector3d>("z"), elems.slice({Dim::X, 2}));
  EXPECT_THROW_DISCARD(vectors.elements<Eigen::Vector3d>("X"),
                       std::out_of_range);
}

class LinalgMatrixTest : public ::testing::Test {
protected:
  Variable matrices = variable::make_matrices(
      Dimensions(Dim::X, 2), sc_units::m,
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19});
};

TEST_F(LinalgMatrixTest, range_check) {
  for (const std::string key :
       {"xx", "xy", "xz", "yx", "yy", "yz", "zx", "zy", "zz"})
    EXPECT_NO_THROW_DISCARD(matrices.elements<Eigen::Matrix3d>(key));
  for (const std::string key : {"x", "y", "z", "XX"})
    EXPECT_THROW_DISCARD(matrices.elements<Eigen::Matrix3d>(key),
                         std::out_of_range);
}
