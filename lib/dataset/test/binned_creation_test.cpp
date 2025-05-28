// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bins.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/creation.h"

using namespace scipp;

class BinnedCreationTest : public ::testing::Test {
protected:
  Variable m_indices = makeVariable<scipp::index_pair>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 5}});
  Variable m_data = makeVariable<double>(Dims{Dim::Event}, Shape{5},
                                         sc_units::m, Values{1, 2, 3, 4, 5});
  DataArray m_buffer =
      DataArray(m_data, {{Dim::X, m_data}}, {{"mask", m_data}});
  Variable m_var = make_bins(m_indices, Dim::Event, m_buffer);

  void check(const Variable &var) const {
    const auto [indices, dim, buf] = var.constituents<DataArray>();
    static_cast<void>(indices);
    static_cast<void>(dim);
    EXPECT_EQ(buf.unit(), sc_units::m);
    EXPECT_TRUE(buf.masks().contains("mask"));
    EXPECT_TRUE(buf.coords().contains(Dim::X));
  }
};

TEST_F(BinnedCreationTest, empty_like_default_shape) {
  const auto empty = empty_like(m_var);
  EXPECT_EQ(empty.dims(), m_var.dims());
  check(empty);
  const auto [indices, dim, buf] = empty.constituents<DataArray>();
  static_cast<void>(dim);
  static_cast<void>(buf);
  EXPECT_EQ(indices, m_indices);
}

TEST_F(BinnedCreationTest, empty_like_slice_default_shape) {
  const auto empty = empty_like(m_var.slice({Dim::X, 1}));
  EXPECT_EQ(empty.dims(), m_var.slice({Dim::X, 1}).dims());
  check(empty);
  const auto [indices, dim, buf] = empty.constituents<DataArray>();
  static_cast<void>(dim);
  static_cast<void>(buf);
  EXPECT_EQ(indices, makeVariable<scipp::index_pair>(Values{std::pair{0, 3}}));
}

TEST_F(BinnedCreationTest, empty_like) {
  Variable shape =
      makeVariable<scipp::index>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                 sc_units::none, Values{1, 2, 5, 6, 3, 4});
  const auto empty = empty_like(m_var, {}, shape);
  EXPECT_EQ(empty.dims(), shape.dims());
  const auto [indices, dim, buf] = empty.constituents<DataArray>();
  static_cast<void>(dim);
  static_cast<void>(indices);
  EXPECT_EQ(buf.dims(), Dimensions(Dim::Event, 21));
  check(empty);
  scipp::index i = 0;
  for (const auto n : {1, 2, 5, 6, 3, 4}) {
    EXPECT_EQ(empty.values<core::bin<DataArray>>()[i++].dims()[Dim::Event], n);
  }
}
