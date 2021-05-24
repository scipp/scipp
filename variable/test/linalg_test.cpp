// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/variable/structures.h"

#include "test_macros.h"

using namespace scipp;

class LinalgVectorTest : public ::testing::Test {
protected:
  Variable vectors = variable::make_vectors(Dimensions(Dim::Y, 2), units::m,
                                            {1, 2, 3, 4, 5, 6});
};

TEST_F(LinalgVectorTest, basics) {
  EXPECT_EQ(vectors.dtype(), dtype<Eigen::Vector3d>);
  EXPECT_EQ(vectors.values<Eigen::Vector3d>()[0], Eigen::Vector3d(1, 2, 3));
  EXPECT_EQ(vectors.values<Eigen::Vector3d>()[1], Eigen::Vector3d(4, 5, 6));
}

TEST_F(LinalgVectorTest, elem_access) {
  Variable elems = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                        units::m, Values{1, 2, 3, 4, 5, 6});
  for (auto i : {0, 1, 2}) {
    EXPECT_EQ(vectors.elements<Eigen::Vector3d>().slice({Dim::Internal0, i}),
              elems.slice({Dim::X, i}));
    EXPECT_EQ(vectors.elements<Eigen::Vector3d>(scipp::index(i)),
              elems.slice({Dim::X, i}));
  }
  EXPECT_THROW_DISCARD(vectors.elements<Eigen::Vector3d>(scipp::index(-1)),
                       std::out_of_range);
  EXPECT_THROW_DISCARD(vectors.elements<Eigen::Vector3d>(scipp::index(3)),
                       std::out_of_range);
}

class LinalgMatrixTest : public ::testing::Test {
protected:
  Variable matrices = variable::make_matrices(
      Dimensions(Dim::X, 2), units::m,
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19});
};

TEST_F(LinalgMatrixTest, range_check) {
  for (scipp::index i : {-1, 0, 1, 2, 3})
    for (scipp::index j : {-1, 0, 1, 2, 3})
      if (i < 0 || i > 2 || j < 0 || j > 2)
        EXPECT_THROW_DISCARD(matrices.elements<Eigen::Matrix3d>(i, j),
                             std::out_of_range);
      else
        EXPECT_NO_THROW_DISCARD(matrices.elements<Eigen::Matrix3d>(i, j));
}
