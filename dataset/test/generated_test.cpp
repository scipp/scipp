// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/math.h"
#include "scipp/variable/math.h"

#include "test_data_arrays.h"

using namespace scipp;

TEST(GeneratedUnaryTest, DataArray) {
  const auto array = make_data_array_1d();
  // Using `reciprocal` as an example of a generatedu nary function
  const auto out = reciprocal(array);
  EXPECT_FALSE(out.data().is_same(array.data()));
  EXPECT_EQ(out.data(), reciprocal(array.data()));
  EXPECT_EQ(out.coords(), array.coords());
  EXPECT_EQ(out.masks(), array.masks());
  EXPECT_EQ(out.attrs(), array.attrs());
  // Meta data is shallow-copied but dicts are not shared
  EXPECT_NE(&out.coords(), &array.coords());
  EXPECT_NE(&out.masks(), &array.masks());
  EXPECT_NE(&out.attrs(), &array.attrs());
  EXPECT_TRUE(out.coords()[Dim::X].is_same(array.coords()[Dim::X]));
  EXPECT_TRUE(out.masks()["mask"].is_same(array.masks()["mask"]));
  EXPECT_TRUE(out.attrs()[Dim("attr")].is_same(array.attrs()[Dim("attr")]));
}
