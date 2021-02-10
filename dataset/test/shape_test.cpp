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

DataArray make_2d_data_array(){
  const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{6, 4},
    Values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24});
  DataArray a(var);
  a.coords().set(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{6},
    Values{1, 2, 3, 4, 5, 6}));
  a.coords().set(Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{4},
    Values{1, 2, 3, 4}));
  a.coords().set(Dim::Z, var);
  a.attrs().set(Dim::Qx, makeVariable<double>(Dims{Dim::X}, Shape{6},
    Values{1.1, 2.1, 3.1, 4.1, 5.1, 6.1}));
  a.attrs().set(Dim::Qy, makeVariable<double>(Dims{Dim::Y}, Shape{4},
    Values{1.2, 2.2, 3.2, 4.2}));
  a.masks().set("mask_x", makeVariable<bool>(Dims{Dim::X}, Shape{6},
    Values{true, true, true, false, false, false}));
  a.masks().set("mask_y", makeVariable<bool>(Dims{Dim::Y}, Shape{4},
    Values{true, true, false, true}));
  // a.masks().set("mask2d", makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{6, 4},
  //   Values{true, true, true, true, true, true,
  //          false, false, false, false, false, false,
  //          true, false, true, false, true, false,
  //          true, true, true, false, false, false}));
  return a;
}

TEST(ReshapeTest, reshape_split_outer) {
  auto a = make_2d_data_array();
  const auto rshp = makeVariable<double>(Dims{Dim::Row, Dim::Tof, Dim::Y},
    Shape{3, 2, 4}, Values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24});
  DataArray expected(rshp);
  expected.coords().set(Dim::Y, a.coords()[Dim::Y]);
  expected.attrs().set(Dim::Qy, a.attrs()[Dim::Qy]);
  expected.masks().set("mask_y", a.masks()["mask_y"]);
  expected.coords().set(Dim::Z, rshp);
  expected.masks().set("mask2d", makeVariable<bool>(Dims{Dim::Row, Dim::Tof, Dim::Y}, Shape{3, 2, 4},
    Values{true, true, true, true, true, true,
           false, false, false, false, false, false,
           true, false, true, false, true, false,
           true, true, true, false, false, false}));
  expected.masks().set("mask_x", makeVariable<bool>(Dims{Dim::Row, Dim::Tof, Dim::Y}, Shape{3, 2, 4},
    Values{true, true, true, true, true, true,
           true, true, true, true, true, true,
           false, false, false, false, false, false,
           false, false, false, false, false, false}));

  EXPECT_EQ(reshape(a, {{Dim::Row, 3}, {Dim::Tof, 2}, {Dim::Y, 4}}), expected);
}


TEST(ReshapeTest, reshape_split_inner) {
  auto a = make_2d_data_array();


  const auto rshp = makeVariable<double>(Dims{Dim::X, Dim::Row, Dim::Tof},
    Shape{6, 2, 2}, Values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24});
  DataArray expected(rshp);
  expected.coords().set(Dim::X, a.coords()[Dim::X]);
  expected.attrs().set(Dim::Qx, a.attrs()[Dim::Qx]);
  expected.masks().set("mask_x", a.masks()["mask_x"]);
  expected.coords().set(Dim::Z, rshp);
  expected.masks().set("mask2d", makeVariable<bool>(Dims{Dim::X, Dim::Row, Dim::Tof}, Shape{6, 2, 2},
    Values{true, true, true, true, true, true,
           false, false, false, false, false, false,
           true, false, true, false, true, false,
           true, true, true, false, false, false}));
  expected.masks().set("mask_y", makeVariable<bool>(Dims{Dim::X, Dim::Row, Dim::Tof}, Shape{6, 2, 2},
    Values{true, true, false, true,
      true, true, false, true,
      true, true, false, true,
      true, true, false, true,
      true, true, false, true,
      true, true, false, true}));

  EXPECT_EQ(reshape(a, {{Dim::X, 6}, {Dim::Row, 2}, {Dim::Tof, 2}}), expected);
}

TEST(ReshapeTest, reshape_merge_dims) {
  auto a = make_2d_data_array();

  const auto rshp = makeVariable<double>(Dims{Dim::Row},
    Shape{24}, Values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24});
  DataArray expected(rshp);
  expected.coords().set(Dim::Z, rshp);
  expected.masks().set("mask_x", makeVariable<bool>(Dims{Dim::Row}, Shape{24},
    Values{true, true, true, true, true, true,
           true, true, true, true, true, true,
           false, false, false, false, false, false,
           false, false, false, false, false, false}));
  expected.masks().set("mask_y", makeVariable<bool>(Dims{Dim::Row}, Shape{24},
    Values{true, true, false, true,
      true, true, false, true,
      true, true, false, true,
      true, true, false, true,
      true, true, false, true,
      true, true, false, true}));
  expected.masks().set("mask2d", makeVariable<bool>(Dims{Dim::Row}, Shape{24},
    Values{true, true, true, true, true, true,
           false, false, false, false, false, false,
           true, false, true, false, true, false,
           true, true, true, false, false, false}));

  EXPECT_EQ(reshape(a, {{Dim::Row, 24}}), expected);
}