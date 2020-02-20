// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

namespace {
auto make_sparse() {
  auto var =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3l, Dimensions::Sparse});
  const auto &var_ = var.sparseValues<double>();
  var_[0] = {1, 2, 3};
  var_[1] = {4, 5};
  var_[2] = {6, 7};
  return var;
}
} // namespace

TEST(ReduceSparseTest, flatten_fail) {
  EXPECT_THROW(static_cast<void>(flatten(make_sparse(), Dim::X)),
               except::DimensionError);
  EXPECT_THROW(static_cast<void>(flatten(make_sparse(), Dim::Z)),
               except::DimensionError);
}

TEST(ReduceSparseTest, flatten) {
  auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{Dimensions::Sparse},
      Values{sparse_container<double>{1, 2, 3, 4, 5, 6, 7}});
  EXPECT_EQ(flatten(make_sparse(), Dim::Y), expected);
}

TEST(ReduceSparseTest, flatten_dataset_with_mask) {
  Dataset d;
  d.setMask("y", makeVariable<bool>(Dims{Dim::Y}, Shape{3},
                                    Values{false, true, false}));
  d.setSparseCoord("a", make_sparse());
  d.setSparseCoord("b", make_sparse());
  d.setSparseLabels("b", "label", make_sparse());
  d.setData("b", make_sparse());
  auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{Dimensions::Sparse},
                           Values{sparse_container<double>{1, 2, 3, 6, 7}});

  const auto flat = flatten(d, Dim::Y);

  EXPECT_EQ(flat["a"].coords()[Dim::X], expected);
  EXPECT_EQ(flat["b"].coords()[Dim::X], expected);
  EXPECT_EQ(flat["b"].labels()["label"], expected);
  EXPECT_EQ(flat["b"].data(), expected);
}
