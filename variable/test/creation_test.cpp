// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/creation.h"
#include "test_variables.h"

using namespace scipp;

TEST_P(DenseVariablesTest, empty_like_fail_if_sizes) {
  const auto var = GetParam();
  EXPECT_THROW(empty_like(var, {}, makeVariable<scipp::index>(Values{12})),
               except::TypeError);
}

TEST_P(DenseVariablesTest, empty_like_default_shape) {
  const auto var = GetParam();
  const auto empty = empty_like(var);
  EXPECT_EQ(empty.dtype(), var.dtype());
  EXPECT_EQ(empty.dims(), var.dims());
  EXPECT_EQ(empty.unit(), var.unit());
  EXPECT_EQ(empty.hasVariances(), var.hasVariances());
}

TEST_P(DenseVariablesTest, empty_like_slice_default_shape) {
  const auto var = GetParam();
  if (var.dims().contains(Dim::X)) {
    const auto empty = empty_like(var.slice({Dim::X, 0}));
    EXPECT_EQ(empty.dtype(), var.dtype());
    EXPECT_EQ(empty.dims(), var.slice({Dim::X, 0}).dims());
    EXPECT_EQ(empty.unit(), var.unit());
    EXPECT_EQ(empty.hasVariances(), var.hasVariances());
  }
}

TEST_P(DenseVariablesTest, empty_like) {
  const auto var = GetParam();
  const Dimensions dims(Dim::X, 4);
  const auto empty = empty_like(var, dims);
  EXPECT_EQ(empty.dtype(), var.dtype());
  EXPECT_EQ(empty.dims(), dims);
  EXPECT_EQ(empty.unit(), var.unit());
  EXPECT_EQ(empty.hasVariances(), var.hasVariances());
}
