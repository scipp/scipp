// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/structures.h"

using namespace scipp;

class VariableStructureTest : public ::testing::Test {
protected:
  Variable vectors = variable::make_vectors(Dimensions(Dim::Y, 2), sc_units::m,
                                            {1, 2, 3, 4, 5, 6});
  Variable matrices = variable::make_matrices(
      Dimensions(Dim::Y, 2), sc_units::m,
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19});
};

TEST_F(VariableStructureTest, basics) {
  EXPECT_EQ(vectors.dtype(), dtype<Eigen::Vector3d>);
  EXPECT_EQ(vectors.values<Eigen::Vector3d>()[0], Eigen::Vector3d(1, 2, 3));
  EXPECT_EQ(vectors.values<Eigen::Vector3d>()[1], Eigen::Vector3d(4, 5, 6));
}

TEST_F(VariableStructureTest, copy) {
  // StructureArrayModel holds a VariableConceptHandle, copy should not share
  auto copied = copy(vectors);
  copied += copied;
  EXPECT_NE(copied, vectors);
}

TEST_F(VariableStructureTest, elem_access) {
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
}

TEST_F(VariableStructureTest, matrices_elem_access) {
  // storage order is column-major
  EXPECT_EQ(
      matrices.elements<Eigen::Matrix3d>("xy"),
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, sc_units::m, Values{4, 14}));
  EXPECT_EQ(
      matrices.elements<Eigen::Matrix3d>("yx"),
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, sc_units::m, Values{2, 12}));
}

TEST_F(VariableStructureTest, elem_access_unit_overwrite) {
  auto elems = vectors.elements<Eigen::Vector3d>();
  EXPECT_EQ(vectors.unit(), sc_units::m);
  EXPECT_EQ(elems.unit(), sc_units::m);
  vectors.setUnit(sc_units::kg);
  EXPECT_EQ(vectors.unit(), sc_units::kg);
  EXPECT_EQ(elems.unit(), sc_units::kg);
  elems.setUnit(sc_units::s);
  EXPECT_EQ(vectors.unit(), sc_units::s);
  EXPECT_EQ(elems.unit(), sc_units::s);
}

TEST_F(VariableStructureTest, readonly) {
  EXPECT_FALSE(vectors.elements<Eigen::Vector3d>().is_readonly());
  EXPECT_TRUE(vectors.as_const().elements<Eigen::Vector3d>().is_readonly());
}

TEST_F(VariableStructureTest, binned) {
  Variable indices = makeVariable<scipp::index_pair>(
      Dimensions(Dim::X, 2), Values{std::pair{0, 1}, std::pair{1, 2}});
  Variable var = make_bins(indices, Dim::Y, vectors);
  Variable elems = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                        sc_units::m, Values{1, 2, 3, 4, 5, 6});
  for (auto x : {0, 1}) {
    for (auto i : {0, 1, 2}) {
      EXPECT_EQ(var.elements<Eigen::Vector3d>()
                    .values<core::bin<Variable>>()[x]
                    .slice({Dim::InternalStructureComponent, i}),
                elems.slice({Dim::X, i}).slice({Dim::Y, x, x + 1}));
    }
    EXPECT_EQ(
        var.elements<Eigen::Vector3d>("x").values<core::bin<Variable>>()[x],
        elems.slice({Dim::X, 0}).slice({Dim::Y, x, x + 1}));
    EXPECT_EQ(
        var.elements<Eigen::Vector3d>("y").values<core::bin<Variable>>()[x],
        elems.slice({Dim::X, 1}).slice({Dim::Y, x, x + 1}));
    EXPECT_EQ(
        var.elements<Eigen::Vector3d>("z").values<core::bin<Variable>>()[x],
        elems.slice({Dim::X, 2}).slice({Dim::Y, x, x + 1}));
  }
}
