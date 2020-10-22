// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/util.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(SizeOf, variable) {
  auto var = makeVariable<double>(units::kg, Shape{4}, Dims{Dim::X},
                                  Values{3, 4, 5, 6});
  EXPECT_EQ(size_of(var), sizeof(double) * 4);

  auto var_with_variance =
      makeVariable<double>(units::kg, Shape{1, 2}, Dims{Dim::X, Dim::Y},
                           Values{3, 4}, Variances{1, 2});

  EXPECT_EQ(size_of(var_with_variance), sizeof(double) * 4);

  auto sliced_view = var.slice(Slice(Dim::X, 0, 2));
  EXPECT_EQ(size_of(sliced_view), 2*sizeof(double));
}

TEST(SizeOf, size_in_memory_for_non_trivial_dtype) {
  auto var = makeVariable<Eigen::Vector3d>(units::kg, Shape{1, 1}, Dims{Dim::X, Dim::Y},
                                  Values{Eigen::Vector3d{1,2,3}});
  EXPECT_EQ(size_of(var), sizeof(Eigen::Vector3d));
}

TEST(SizeOf, size_in_memory_sliced_variables) {
  auto var = makeVariable<double>(units::kg, Shape{4}, Dims{Dim::X},
                                  Values{3, 4, 5, 6});
  auto sliced_view = var.slice(Slice(Dim::X, 0, 2));
  EXPECT_EQ(size_of(sliced_view), 2*sizeof(double));
}
