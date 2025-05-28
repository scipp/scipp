// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"
#include "test_util.h"

#include "scipp/dataset/shape.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/shape.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::dataset;

TEST(ResizeTest, data_array_1d) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  DataArray a(var);
  a.coords().set(Dim::X, var);
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
  a.masks().set("mask-x", x);
  a.masks().set("mask-y", y);

  DataArray expected(makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{1, 2}));
  expected.coords().set(Dim::X, x);
  expected.masks().set("mask-x", x);

  EXPECT_EQ(resize(a, Dim::Y, 1), expected);
  EXPECT_NE(resize(a, Dim::Y, 1).masks()["mask-x"].values<double>().data(),
            expected.masks()["mask-x"].values<double>().data());

  Dataset d({{"a", a}});
  Dataset expected_d({{"a", expected}});
  EXPECT_EQ(resize(d, Dim::Y, 1), expected_d);
  EXPECT_NE(resize(d, Dim::Y, 1)["a"].masks()["mask-x"].values<double>().data(),
            expected_d["a"].masks()["mask-x"].values<double>().data());
}

TEST(ReshapeTest, fold_x) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}});
  DataArray expected(rshp);
  expected.coords().set(
      Dim::X, fold(arange(Dim::X, 6), Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}) +
                  0.1 * sc_units::one);
  expected.coords().set(Dim::Y, a.coords()[Dim::Y]);

  EXPECT_EQ(fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}), expected);
}

TEST(ReshapeTest, fold_y) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::X, 6}, {Dim::Row, 2}, {Dim::Time, 2}});
  DataArray expected(rshp);
  expected.coords().set(
      Dim::Y, fold(arange(Dim::Y, 4), Dim::Y, {{Dim::Row, 2}, {Dim::Time, 2}}) +
                  0.2 * sc_units::one);
  expected.coords().set(Dim::X, a.coords()[Dim::X]);

  EXPECT_EQ(fold(a, Dim::Y, {{Dim::Row, 2}, {Dim::Time, 2}}), expected);
}

TEST(ReshapeTest, fold_into_3_dims) {
  const auto var = arange(Dim::X, 24);
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 24) + 0.1 * sc_units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::Time, 2}, {Dim::Y, 3}, {Dim::Z, 4}});
  DataArray expected(rshp);
  expected.coords().set(Dim::X, rshp + 0.1 * sc_units::one);

  EXPECT_EQ(fold(a, Dim::X, {{Dim::Time, 2}, {Dim::Y, 3}, {Dim::Z, 4}}),
            expected);
}

TEST(ReshapeTest, flatten) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);
  a.coords().set(Dim{"xy"}, 0.3 * sc_units::one + var);
  a.coords().set(Dim{"yx"}, copy(transpose(0.4 * sc_units::one + var,
                                           std::vector{Dim{"y"}, Dim{"x"}})));

  const auto expected_data = arange(Dim::Z, 24);
  DataArray expected(expected_data);
  expected.coords().set(
      Dim::X,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.1, 0.1, 0.1, 0.1, 1.1, 1.1, 1.1, 1.1,
                                  2.1, 2.1, 2.1, 2.1, 3.1, 3.1, 3.1, 3.1,
                                  4.1, 4.1, 4.1, 4.1, 5.1, 5.1, 5.1, 5.1}));
  expected.coords().set(
      Dim::Y,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2}));
  expected.coords().set(Dim{"xy"}, 0.3 * sc_units::one + expected_data);
  expected.coords().set(Dim{"yx"}, 0.4 * sc_units::one + expected_data);

  EXPECT_EQ(flatten(a, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z), expected);
}

TEST(ReshapeTest, flatten_single_dim) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);
  a.coords().set(Dim{"xy"}, 0.3 * sc_units::one + var);
  a.coords().set(Dim{"yx"}, copy(transpose(0.4 * sc_units::one + var,
                                           std::vector<Dim>{Dim::Y, Dim::X})));

  EXPECT_EQ(flatten(a, std::vector<Dim>{Dim::X}, Dim::Z),
            a.rename_dims(std::vector{std::pair{Dim{"x"}, Dim{"z"}}}));
  EXPECT_EQ(flatten(a, std::vector<Dim>{Dim::Y}, Dim::Z),
            a.rename_dims(std::vector{std::pair{Dim{"y"}, Dim{"z"}}}));
}

TEST(ReshapeTest, flatten_all_dims) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);
  a.coords().set(Dim("scalar"), makeVariable<double>(Values{1.2}));

  const auto flat = flatten(a, std::nullopt, Dim::Z);
  EXPECT_EQ(flat.data(),
            flatten(var, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z));
  EXPECT_EQ(flat.coords()[Dim::X].dims(), flat.data().dims());
  EXPECT_EQ(flat.coords()[Dim::Y].dims(), flat.data().dims());
  EXPECT_EQ(flat.coords()[Dim("scalar")], a.coords()[Dim("scalar")]);
}

TEST(ReshapeTest, flatten_dim_not_in_input) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);

  EXPECT_THROW_DISCARD(flatten(a, std::vector<Dim>{Dim::Time}, Dim::Z),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(flatten(a, std::vector<Dim>{Dim::X, Dim::Time}, Dim::Z),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(flatten(a, std::vector<Dim>{Dim::Time, Dim::X}, Dim::Z),
                       except::DimensionError);
}

TEST(ReshapeTest, flatten_bad_dim_order) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);

  EXPECT_THROW_MSG_DISCARD(
      flatten(a, std::vector<Dim>{Dim::Y, Dim::X}, Dim::Z),
      except::DimensionError,
      "Can only flatten a contiguous set of dimensions in the correct order");
}

TEST(ReshapeTest, flatten_empty_from_arg) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);

  DataArray expected(
      broadcast(var, Dimensions{{Dim::X, Dim::Y, Dim::Z}, {6, 4, 1}}));
  expected.coords().set(Dim::X, a.coords()[Dim::X]);
  expected.coords().set(Dim::Y, a.coords()[Dim::Y]);

  EXPECT_EQ(flatten(a, std::vector<Dim>{}, Dim::Z), expected);
}

TEST(ReshapeTest, flatten_scalar_no_dims) {
  const auto var = makeVariable<double>(Dims{}, Shape{}, Values{2.0});
  DataArray a(var);
  a.coords().set(Dim::X, 0.1 * sc_units::one + var);

  DataArray expected(makeVariable<double>(Dims{Dim::Z}, Shape{1}, Values{2.0}));
  expected.coords().set(Dim::X, a.coords()[Dim::X]);

  EXPECT_EQ(flatten(a, std::vector<Dim>{}, Dim::Z), expected);
}

TEST(ReshapeTest, flatten_scalar_all_dims) {
  const auto var = makeVariable<double>(Dims{}, Shape{}, Values{2.0});
  DataArray a(var);
  a.coords().set(Dim::X, 0.1 * sc_units::one + var);

  DataArray expected(makeVariable<double>(Dims{Dim::Z}, Shape{1}, Values{2.0}));
  expected.coords().set(Dim::X, 0.1 * sc_units::one + expected.data());

  EXPECT_EQ(flatten(a, std::nullopt, Dim::Z), expected);
}

TEST(ReshapeTest, round_trip) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);

  auto reshaped = fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}});
  EXPECT_EQ(flatten(reshaped, std::vector<Dim>{Dim::Row, Dim::Time}, Dim::X),
            a);
}

TEST(ReshapeTest, fold_x_binedges_x) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 7) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}});
  DataArray expected(rshp);
  expected.coords().set(
      Dim::X,
      makeVariable<double>(Dims{Dim::Row, Dim::Time}, Shape{2, 4},
                           Values{0.1, 1.1, 2.1, 3.1, 3.1, 4.1, 5.1, 6.1}));
  expected.coords().set(Dim::Y, a.coords()[Dim::Y]);

  EXPECT_EQ(fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}), expected);
}

TEST(ReshapeTest, fold_y_binedges_y) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 5) + 0.2 * sc_units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::X, 6}, {Dim::Row, 2}, {Dim::Time, 2}});
  DataArray expected(rshp);
  expected.coords().set(Dim::X, a.coords()[Dim::X]);
  expected.coords().set(
      Dim::Y, makeVariable<double>(Dims{Dim::Row, Dim::Time}, Shape{2, 3},
                                   Values{0.2, 1.2, 2.2, 2.2, 3.2, 4.2}));

  EXPECT_EQ(fold(a, Dim::Y, {{Dim::Row, 2}, {Dim::Time, 2}}), expected);
}

TEST(ReshapeTest, flatten_binedges_1d) {
  DataArray a(arange(Dim::X, 4), {{Dim::Z, arange(Dim::X, 5)}});
  const auto flat = flatten(a, std::vector<Dim>{Dim::X}, Dim::Y);
  const auto expected = a.rename_dims({{Dim::X, Dim::Y}});
  EXPECT_EQ(flat, expected);
}

TEST(ReshapeTest, flatten_drops_unjoinable_outer_binedges) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 7) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);

  const auto flat = flatten(a, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z);
  EXPECT_FALSE(flat.coords().contains(Dim::X));
}

TEST(ReshapeTest, flatten_drops_unjoinable_inner_binedges) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 5) + 0.2 * sc_units::one);

  const auto flat = flatten(a, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z);
  EXPECT_FALSE(flat.coords().contains(Dim::Y));
}

TEST(ReshapeTest, round_trip_binedges) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 7) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);

  auto reshaped = fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}});
  EXPECT_EQ(flatten(reshaped, std::vector<Dim>{Dim::Row, Dim::Time}, Dim::X),
            a);
}

TEST(ReshapeTest, fold_x_with_2d_coord) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X,
                 fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}}) +
                     0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}});
  DataArray expected(rshp);
  expected.coords().set(Dim::X,
                        fold(arange(Dim::X, 24), Dim::X,
                             {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}}) +
                            0.1 * sc_units::one);
  expected.coords().set(Dim::Y, a.coords()[Dim::Y]);

  EXPECT_EQ(fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}), expected);
}

TEST(ReshapeTest, flatten_with_2d_coord) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X,
                 fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}}) +
                     0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);

  const auto rshp = arange(Dim::Z, 24);
  DataArray expected(rshp);
  expected.coords().set(Dim::X, arange(Dim::Z, 24) + 0.1 * sc_units::one);
  expected.coords().set(
      Dim::Y,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2}));

  EXPECT_EQ(flatten(a, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z), expected);
}

TEST(ReshapeTest, fold_x_with_masks) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);
  a.masks().set("mask_x", makeVariable<bool>(
                              Dims{Dim::X}, Shape{6},
                              Values{true, true, true, false, false, false}));
  a.masks().set("mask_y", makeVariable<bool>(Dims{Dim::Y}, Shape{4},
                                             Values{true, true, false, true}));
  a.masks().set(
      "mask2d",
      makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{6, 4},
                         Values{true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                true,  false, true,  false, true,  false,
                                true,  true,  true,  false, false, false}));

  const auto rshp = fold(arange(Dim::X, 24), Dim::X,
                         {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}});
  DataArray expected(rshp);
  expected.coords().set(
      Dim::X, fold(arange(Dim::X, 6), Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}) +
                  0.1 * sc_units::one);
  expected.coords().set(Dim::Y, a.coords()[Dim::Y]);
  expected.masks().set(
      "mask_x",
      makeVariable<bool>(Dims{Dim::Row, Dim::Time}, Shape{2, 3},
                         Values{true, true, true, false, false, false}));
  expected.masks().set("mask_y",
                       makeVariable<bool>(Dims{Dim::Y}, Shape{4},
                                          Values{true, true, false, true}));
  expected.masks().set(
      "mask2d",
      makeVariable<bool>(Dims{Dim::Row, Dim::Time, Dim::Y}, Shape{2, 3, 4},
                         Values{true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                true,  false, true,  false, true,  false,
                                true,  true,  true,  false, false, false}));

  EXPECT_EQ(fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}), expected);
}

TEST(ReshapeTest, flatten_with_masks) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 6) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);
  a.masks().set("mask_x", makeVariable<bool>(
                              Dims{Dim::X}, Shape{6},
                              Values{true, true, true, false, false, false}));
  a.masks().set("mask_y", makeVariable<bool>(Dims{Dim::Y}, Shape{4},
                                             Values{true, true, false, true}));
  a.masks().set(
      "mask2d",
      makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{6, 4},
                         Values{true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                true,  false, true,  false, true,  false,
                                true,  true,  true,  false, false, false}));

  const auto rshp = arange(Dim::Z, 24);
  DataArray expected(rshp);
  expected.coords().set(
      Dim::X,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.1, 0.1, 0.1, 0.1, 1.1, 1.1, 1.1, 1.1,
                                  2.1, 2.1, 2.1, 2.1, 3.1, 3.1, 3.1, 3.1,
                                  4.1, 4.1, 4.1, 4.1, 5.1, 5.1, 5.1, 5.1}));
  expected.coords().set(
      Dim::Y,
      makeVariable<double>(Dims{Dim::Z}, Shape{24},
                           Values{0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2,
                                  0.2, 1.2, 2.2, 3.2, 0.2, 1.2, 2.2, 3.2}));

  expected.masks().set(
      "mask_x",
      makeVariable<bool>(Dims{Dim::Z}, Shape{24},
                         Values{true,  true,  true,  true,  true,  true,
                                true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                false, false, false, false, false, false}));
  expected.masks().set(
      "mask_y", makeVariable<bool>(
                    Dims{Dim::Z}, Shape{24},
                    Values{true, true, false, true, true, true, false, true,
                           true, true, false, true, true, true, false, true,
                           true, true, false, true, true, true, false, true}));
  expected.masks().set(
      "mask2d",
      makeVariable<bool>(Dims{Dim::Z}, Shape{24},
                         Values{true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                true,  false, true,  false, true,  false,
                                true,  true,  true,  false, false, false}));

  EXPECT_EQ(flatten(a, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z), expected);
}

TEST(ReshapeTest, round_trip_with_all) {
  const auto var = fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}});
  DataArray a(var);
  a.coords().set(Dim::X, arange(Dim::X, 7) + 0.1 * sc_units::one);
  a.coords().set(Dim::Y, arange(Dim::Y, 4) + 0.2 * sc_units::one);
  a.coords().set(Dim::Z,
                 fold(arange(Dim::X, 24), Dim::X, {{Dim::X, 6}, {Dim::Y, 4}}) +
                     0.5 * sc_units::one);
  a.masks().set("mask_x", makeVariable<bool>(
                              Dims{Dim::X}, Shape{6},
                              Values{true, true, true, false, false, false}));
  a.masks().set("mask_y", makeVariable<bool>(Dims{Dim::Y}, Shape{4},
                                             Values{true, true, false, true}));
  a.masks().set(
      "mask2d",
      makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{6, 4},
                         Values{true,  true,  true,  true,  true,  true,
                                false, false, false, false, false, false,
                                true,  false, true,  false, true,  false,
                                true,  true,  true,  false, false, false}));
  auto reshaped = fold(a, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}});
  EXPECT_EQ(flatten(reshaped, std::vector<Dim>{Dim::Row, Dim::Time}, Dim::X),
            a);
}

class TransposeTest : public ::testing::Test {
protected:
  TransposeTest() : a(xy) {
    a.coords().set(Dim::X, x);
    a.coords().set(Dim::Y, y);
    a.masks().set("mask-x", x);
    a.masks().set("mask-y", y);
    a.masks().set("mask-xy", xy);
  }
  Variable xy = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 2},
                                     Values{1, 2, 3, 4, 5, 6});
  Variable x = xy.slice({Dim::Y, 0});
  Variable y = xy.slice({Dim::X, 0});
  DataArray a;
};

TEST_F(TransposeTest, data_array_2d) {
  auto transposed = transpose(a);
  EXPECT_EQ(transposed.data(), transpose(a.data()));
  transposed.setData(a.data());
  EXPECT_EQ(transposed, a);
  EXPECT_EQ(transpose(a, std::vector<Dim>{Dim::X, Dim::Y}), transpose(a));
  EXPECT_EQ(transpose(a, std::vector<Dim>{Dim::Y, Dim::X}), a);
}

TEST_F(TransposeTest, data_array_2d_meta_data) {
  Variable edges = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3},
                                        Values{1, 2, 3, 4, 5, 6, 7, 8, 9});
  // Note: The 2-d coord must not be transposed, since this would break the
  // association with its dimension. Mask may in principle be transposed but is
  // not right now.
  a.coords().set(Dim("edges"), edges);
  a.masks().set("mask", xy);
  auto transposed = transpose(a);
  EXPECT_EQ(transposed.data(), transpose(a.data()));
  transposed.setData(a.data());
  EXPECT_EQ(transposed, a);
  EXPECT_EQ(transpose(a, std::vector<Dim>{Dim::X, Dim::Y}), transpose(a));
  EXPECT_EQ(transpose(a, std::vector<Dim>{Dim::Y, Dim::X}), a);
}

TEST_F(TransposeTest, dataset_no_order) {
  Dataset d({{"a", a}, {"b", transpose(a)}});
  // Slightly unusual but "simple" behavior if no dim order given
  auto transposed = transpose(d);
  EXPECT_EQ(transposed["a"], d["b"]);
  EXPECT_EQ(transposed["b"], d["a"]);
}

TEST_F(TransposeTest, dataset_2d) {
  Dataset d({{"a", a}, {"b", transpose(a)}});
  auto transposed = transpose(d, std::vector<Dim>{Dim::X, Dim::Y});
  EXPECT_EQ(transposed["a"], d["b"]);
  EXPECT_EQ(transposed["b"], d["b"]);
}

class SqueezeTest : public ::testing::Test {
protected:
  SqueezeTest() : a(xyz) {
    a.coords().set(Dim::X, x);
    a.coords().set(Dim::Y, y);
    a.coords().set(Dim::Z, z);
    a.coords().set(Dim{"xyz"}, xyz);
    a.masks().set("mask-x", x);
    a.masks().set("mask-z", z);
    a.masks().set("mask-xyz", xyz);
  }
  Variable xyz = makeVariable<double>(Dims{Dim::X, Dim::Y, Dim::Z},
                                      Shape{1, 1, 2}, Values{1, 2});
  Variable x = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{10});
  Variable y = makeVariable<double>(Dims{Dim::Y}, Shape{1}, Values{20});
  Variable z = makeVariable<double>(Dims{Dim::Z}, Shape{2}, Values{30, 31});
  DataArray a;
};

TEST_F(SqueezeTest, data_array_3d_outer) {
  const auto dims = std::vector<Dim>{Dim::X};
  const auto squeezed = squeeze(a, dims);
  EXPECT_EQ(squeezed.data(), squeeze(xyz, dims));
  EXPECT_EQ(squeezed.coords()[Dim::X], makeVariable<double>(Values{10}));
  EXPECT_FALSE(squeezed.coords()[Dim::X].is_aligned());
  EXPECT_EQ(squeezed.coords()[Dim::Y], y);
  EXPECT_EQ(squeezed.coords()[Dim::Z], z);
  EXPECT_EQ(squeezed.coords()[Dim{"xyz"}], squeeze(xyz, dims));
  EXPECT_EQ(squeezed.masks()["mask-x"], makeVariable<double>(Values{10}));
  EXPECT_EQ(squeezed.masks()["mask-z"], z);
  EXPECT_EQ(squeezed.masks()["mask-xyz"], squeeze(xyz, dims));
}

TEST_F(SqueezeTest, data_array_3d_center) {
  const auto dims = std::vector<Dim>{Dim::Y};
  const auto squeezed = squeeze(a, dims);
  EXPECT_EQ(squeezed.data(), squeeze(xyz, dims));
  EXPECT_EQ(squeezed.coords()[Dim::X], x);
  EXPECT_EQ(squeezed.coords()[Dim::Y], makeVariable<double>(Values{20}));
  EXPECT_FALSE(squeezed.coords()[Dim::Y].is_aligned());
  EXPECT_EQ(squeezed.coords()[Dim::Z], z);
  EXPECT_EQ(squeezed.coords()[Dim{"xyz"}], squeeze(xyz, dims));
  EXPECT_EQ(squeezed.masks()["mask-x"], x);
  EXPECT_EQ(squeezed.masks()["mask-z"], z);
  EXPECT_EQ(squeezed.masks()["mask-xyz"], squeeze(xyz, dims));
}

TEST_F(SqueezeTest, data_array_3d_inner_and_center) {
  const auto dims = std::vector<Dim>{Dim::Y, Dim::X};
  const auto squeezed = squeeze(a, dims);
  EXPECT_EQ(squeezed.data(), squeeze(xyz, dims));
  EXPECT_EQ(squeezed.coords()[Dim::X], makeVariable<double>(Values{10}));
  EXPECT_FALSE(squeezed.coords()[Dim::X].is_aligned());
  EXPECT_EQ(squeezed.coords()[Dim::Y], makeVariable<double>(Values{20}));
  EXPECT_FALSE(squeezed.coords()[Dim::Y].is_aligned());
  EXPECT_EQ(squeezed.coords()[Dim::Z], z);
  EXPECT_EQ(squeezed.coords()[Dim{"xyz"}], squeeze(xyz, dims));
  EXPECT_EQ(squeezed.masks()["mask-x"], makeVariable<double>(Values{10}));
  EXPECT_EQ(squeezed.masks()["mask-z"], z);
  EXPECT_EQ(squeezed.masks()["mask-xyz"], squeeze(xyz, dims));
}

TEST_F(SqueezeTest, data_array_3d_outer_bin_edge) {
  const auto dims = std::vector<Dim>{Dim::X};
  a.coords().set(Dim::X,
                 makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{-1, -2}));
  const auto squeezed = squeeze(a, dims);
  EXPECT_EQ(squeezed.data(), squeeze(xyz, dims));
  EXPECT_EQ(squeezed.coords()[Dim::X],
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{-1, -2}));
  EXPECT_FALSE(squeezed.coords()[Dim::X].is_aligned());
  EXPECT_EQ(squeezed.coords()[Dim::Y], y);
  EXPECT_EQ(squeezed.coords()[Dim::Z], z);
  EXPECT_EQ(squeezed.coords()[Dim{"xyz"}], squeeze(xyz, dims));
  EXPECT_EQ(squeezed.masks()["mask-x"], makeVariable<double>(Values{10}));
  EXPECT_EQ(squeezed.masks()["mask-z"], z);
  EXPECT_EQ(squeezed.masks()["mask-xyz"], squeeze(xyz, dims));
}

TEST_F(SqueezeTest, data_array_3d_all) {
  EXPECT_EQ(squeeze(a, std::nullopt),
            squeeze(a, std::vector<Dim>{Dim::X, Dim::Y}));
}

TEST_F(SqueezeTest, data_array_3d_no_dims) {
  const auto dims = std::vector<Dim>{};
  const auto squeezed = squeeze(a, dims);
  EXPECT_EQ(squeezed, a);
}

TEST_F(SqueezeTest, data_array_3d_wrong_length_throws) {
  EXPECT_THROW_DISCARD(squeeze(a, std::vector<Dim>{Dim::Z}),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(squeeze(a, std::vector<Dim>{Dim::X, Dim::Z}),
                       except::DimensionError);
}

TEST_F(SqueezeTest, data_array_output_is_not_readonly) {
  auto squeezed = squeeze(a, std::nullopt);
  EXPECT_NO_THROW_DISCARD(squeezed.data().setSlice(
      Slice{Dim::Z, 0}, makeVariable<double>(Dims{}, Values{-10.0})));
  EXPECT_NO_THROW_DISCARD(squeezed.coords().erase(Dim::Z));
}

class SqueezeDatasetTest : public SqueezeTest {
protected:
  SqueezeDatasetTest() : dset{{{"a", a}}} {}

  Dataset dset;
};

TEST_F(SqueezeDatasetTest, dataset_3d_outer) {
  const std::vector<Dim> dims{Dim::X};
  const auto squeezed = squeeze(dset, dims);
  EXPECT_EQ(squeezed["a"], squeeze(a, dims));
}

TEST_F(SqueezeDatasetTest, dataset_3d_center) {
  const std::vector<Dim> dims{Dim::Y};
  const auto squeezed = squeeze(dset, dims);
  EXPECT_EQ(squeezed["a"], squeeze(a, dims));
}

TEST_F(SqueezeDatasetTest, dataset_3d_all) {
  const auto dims = std::nullopt;
  const auto squeezed = squeeze(dset, dims);
  EXPECT_EQ(squeezed["a"], squeeze(a, dims));
}

TEST_F(SqueezeDatasetTest, dataset_3d_no_dims) {
  const std::vector<Dim> dims{};
  const auto squeezed = squeeze(dset, dims);
  EXPECT_EQ(squeezed, dset);
}

TEST_F(SqueezeDatasetTest, dataset_output_is_not_readonly) {
  auto squeezed = squeeze(dset, std::nullopt);
  EXPECT_NO_THROW_DISCARD(squeezed["a"].data().setSlice(
      Slice{Dim::Z, 0}, makeVariable<double>(Dims{}, Values{-10.0})));
  EXPECT_NO_THROW_DISCARD(squeezed.coords().erase(Dim::Z));
}
