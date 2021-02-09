// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/shape.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(ResizeTest, data_array_1d) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  DataArray a(var);
  a.coords().set(Dim::X, var);
  a.attrs().set(Dim::Y, var);
  a.masks().set("mask", var);
  DataArray expected(makeVariable<double>(Dims{Dim::X}, Shape{3}));
  EXPECT_EQ(resize(a, Dim::X, 3), expected);
}

TEST(ResizeTest, data_array_2d) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 2},
                                        Values{1, 2, 3, 4, 5, 6});
  auto x = var.slice({Dim::Y, 0});
  auto y = var.slice({Dim::X, 0});
  DataArray a(var);
  a.coords().set(Dim::X, x);
  a.coords().set(Dim::Y, y);
  a.attrs().set(Dim("unaligned-x"), x);
  a.attrs().set(Dim("unaligned-y"), y);
  a.masks().set("mask-x", x);
  a.masks().set("mask-y", y);

  DataArray expected(makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{1, 2}));
  expected.coords().set(Dim::X, x);
  expected.attrs().set(Dim("unaligned-x"), x);
  expected.masks().set("mask-x", x);

  EXPECT_EQ(resize(a, Dim::Y, 1), expected);

  Dataset d({{"a", a}});
  Dataset expected_d({{"a", expected}});
  EXPECT_EQ(resize(d, Dim::Y, 1), expected_d);
}

TEST(ReshapeTest, reshape) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  DataArray a(var);
  a.coords().set(Dim::X, var);
  a.attrs().set(Dim::Tof, var);
  a.masks().set("mask", var);
  DataArray expected(makeVariable<double>(Dims{Dim::Z, Dim::Y}, Shape{3, 2}, Values{1, 2, 3, 4, 5, 6}));
  EXPECT_EQ(reshape(a, {{Dim::Z, 3}, {Dim::Y, 2}}), expected);


  // const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
  //                                       units::m, Values{1, 2, 3, 4, 5, 6});

  // ASSERT_EQ(reshape(var, {Dim::Row, 6}),
  //           makeVariable<double>(Dims{Dim::Row}, Shape{6}, units::m,
  //                                Values{1, 2, 3, 4, 5, 6}));
  // ASSERT_EQ(reshape(var, {{Dim::Row, 3}, {Dim::Z, 2}}),
  //           makeVariable<double>(Dims{Dim::Row, Dim::Z}, Shape{3, 2}, units::m,
  //                                Values{1, 2, 3, 4, 5, 6}));
}