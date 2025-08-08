// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"
#include "test_util.h"

#include "scipp/core/except.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/shape.h"

using namespace scipp;

TEST(ShapeTest, broadcast) {
  auto reference =
      makeVariable<double>(Dims{Dim::Z, Dim::Y, Dim::X}, Shape{3, 2, 2},
                           Values{1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4},
                           Variances{5, 6, 7, 8, 5, 6, 7, 8, 5, 6, 7, 8});
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1, 2, 3, 4}, Variances{5, 6, 7, 8});
  EXPECT_EQ(broadcast(var, var.dims()), var);
  EXPECT_EQ(broadcast(var, transpose(var.dims())), transpose(var));
  const Dimensions z(Dim::Z, 3);
  EXPECT_EQ(broadcast(var, merge(z, var.dims())), reference);
  EXPECT_EQ(broadcast(var, merge(var.dims(), z)),
            transpose(reference, std::vector<Dim>{Dim::Y, Dim::X, Dim::Z}));
}

TEST(ShapeTest, broadcast_does_not_copy) {
  auto scalar = makeVariable<double>(Values{1});
  auto var = broadcast(scalar, {Dim::X, 2});
  // cppcheck-suppress unreadVariable  # Read through `var`.
  scalar += scalar;
  EXPECT_EQ(var, makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 2}));
}

TEST(ShapeTest, broadcast_output_is_readonly) {
  auto var = broadcast(makeVariable<double>(Values{1}), {Dim::X, 2});
  EXPECT_TRUE(var.is_readonly());
}

TEST(ShapeTest, broadcast_output_is_not_readonly_if_not_broadcast) {
  auto var = broadcast(makeVariable<double>(Values{1}), {Dim::X, 1});
  EXPECT_FALSE(var.is_readonly());
}

TEST(ShapeTest, broadcast_fail) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1, 2, 3, 4});
  EXPECT_THROW_DISCARD(broadcast(var, {Dim::X, 3}), except::DimensionError);
}

class SqueezeTest : public ::testing::Test {
protected:
  Variable var = makeVariable<double>(Dims{Dim::X, Dim::Y, Dim::Z},
                                      Shape{1, 2, 1}, Values{1, 2});
  Variable original = var;
};

TEST_F(SqueezeTest, fail) {
  EXPECT_THROW_DISCARD(squeeze(var, std::vector<Dim>{Dim::Y}),
                       except::DimensionError);
  EXPECT_EQ(var, original);
  EXPECT_THROW_DISCARD(squeeze(var, std::vector<Dim>{Dim::X, Dim::Y}),
                       except::DimensionError);
  EXPECT_EQ(var, original);
  EXPECT_THROW_DISCARD(squeeze(var, std::vector<Dim>{Dim::Y, Dim::Z}),
                       except::DimensionError);
  EXPECT_EQ(var, original);
}

TEST_F(SqueezeTest, none) {
  EXPECT_EQ(squeeze(var, std::vector<Dim>{}), original);
}

TEST_F(SqueezeTest, outer) {
  EXPECT_EQ(squeeze(var, std::vector<Dim>{Dim::X}), sum(original, Dim::X));
}

TEST_F(SqueezeTest, inner) {
  EXPECT_EQ(squeeze(var, std::vector<Dim>{Dim::Z}), sum(original, Dim::Z));
}

TEST_F(SqueezeTest, both) {
  EXPECT_EQ(squeeze(var, std::vector<Dim>{Dim::X, Dim::Z}),
            sum(sum(original, Dim::Z), Dim::X));
}

TEST_F(SqueezeTest, all) {
  EXPECT_EQ(squeeze(var, std::nullopt),
            squeeze(var, std::vector<Dim>{Dim::X, Dim::Z}));
}

TEST_F(SqueezeTest, to_scalar) {
  var = var.slice({Dim::Y, 0});
  EXPECT_EQ(squeeze(var, std::nullopt), makeVariable<double>(Values{1}));
}

TEST_F(SqueezeTest, all_var_has_no_length_1) {
  const auto v = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                      Values{1, 2, 3, 4});
  EXPECT_EQ(squeeze(v, std::nullopt), v);
}

TEST_F(SqueezeTest, slice) {
  Variable xy = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                     Values{1, 2, 3, 4});
  const auto sliced = xy.slice({Dim::Y, 1, 2});
  EXPECT_EQ(squeeze(sliced, std::vector<Dim>{Dim::Y}), sum(sliced, Dim::Y));
}

TEST_F(SqueezeTest, shares_buffer) {
  auto squeezed = squeeze(var, std::nullopt);
  const core::Slice slice{Dim::Y, 0};
  squeezed.setSlice(slice, makeVariable<double>(Values{-1}));
  EXPECT_EQ(sum(sum(var, Dim::X), Dim::Z).slice(slice),
            makeVariable<double>(Values{-1}));
}

TEST(ShapeTest, fold_fail_if_dim_not_found) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{4});
  EXPECT_THROW_DISCARD(fold(var, Dim::Time, {{Dim::Y, 2}, {Dim::Z, 2}}),
                       except::NotFoundError);
}

TEST(ShapeTest, fold_does_not_copy) {
  const auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  const auto expected = var + var;
  auto folded = fold(var, Dim::X, {{Dim::Y, 2}, {Dim::Z, 2}});
  // cppcheck-suppress unreadVariable  # Read through `var`.
  folded += folded;
  EXPECT_EQ(var, expected);
}

TEST(ShapeTest, fold_temporary) {
  auto var = fold(makeVariable<double>(Dims{Dim::X}, Shape{4}), Dim::X,
                  {{Dim::Y, 2}, {Dim::Z, 2}});
  EXPECT_EQ(var.data_handle().use_count(), 1);
}

TEST(ShapeTest, fold_outer) {
  const auto var = cumsum(
      variable::ones({{Dim::X, 6}, {Dim::Y, 4}}, sc_units::m, dtype<double>));
  const auto expected =
      cumsum(variable::ones({{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}},
                            sc_units::m, dtype<double>));
  EXPECT_EQ(fold(var, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}), expected);
}

TEST(ShapeTest, fold_inner) {
  const auto var = cumsum(
      variable::ones({{Dim::X, 6}, {Dim::Y, 4}}, sc_units::m, dtype<double>));
  const auto expected =
      cumsum(variable::ones({{Dim::X, 6}, {Dim::Row, 2}, {Dim::Time, 2}},
                            sc_units::m, dtype<double>));
  EXPECT_EQ(fold(var, Dim::Y, {{Dim::Row, 2}, {Dim::Time, 2}}), expected);
}

TEST(ShapeTest, fold_into_3_dims) {
  const auto var =
      cumsum(variable::ones({{Dim::X, 24}}, sc_units::m, dtype<double>));
  const auto expected = cumsum(variable::ones(
      {{Dim::Time, 2}, {Dim::Y, 3}, {Dim::Z, 4}}, sc_units::m, dtype<double>));
  EXPECT_EQ(fold(var, Dim::X, {{Dim::Time, 2}, {Dim::Y, 3}, {Dim::Z, 4}}),
            expected);
}

TEST(ShapeTest, flatten) {
  const auto var = cumsum(
      variable::ones({{Dim::X, 6}, {Dim::Y, 4}}, sc_units::m, dtype<double>));
  const auto expected =
      cumsum(variable::ones({{Dim::Z, 24}}, sc_units::m, dtype<double>));
  const auto flat = flatten(var, std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z);
  EXPECT_EQ(flat, expected);
  EXPECT_EQ(flat.data_handle(), var.data_handle()); // shared
}

TEST(ShapeTest, flatten_nothing) {
  const auto var =
      cumsum(variable::ones({{Dim::X, 4}}, sc_units::m, dtype<double>));
  const auto expected = cumsum(
      variable::ones({{Dim::X, 4}, {Dim::Y, 1}}, sc_units::m, dtype<double>));
  const auto flat = flatten(var, std::vector<Dim>{}, Dim::Y);
  EXPECT_EQ(flat, expected);
  EXPECT_EQ(flat.data_handle(), var.data_handle()); // shared
  EXPECT_FALSE(flat.is_readonly()); // broadcast, but same size => writeable
}

TEST(ShapeTest, flatten_only_2_dims) {
  const auto var = cumsum(variable::ones(
      {{Dim::X, 2}, {Dim::Y, 3}, {Dim::Z, 4}}, sc_units::m, dtype<double>));
  const auto expected = cumsum(
      variable::ones({{Dim::X, 6}, {Dim::Z, 4}}, sc_units::m, dtype<double>));
  EXPECT_EQ(flatten(var, std::vector<Dim>{Dim::X, Dim::Y}, Dim::X), expected);
}

TEST(ShapeTest, flatten_slice) {
  const auto var = cumsum(
      variable::ones({{Dim::X, 4}, {Dim::Y, 5}}, sc_units::m, dtype<double>));
  const auto expected = makeVariable<double>(
      Dims{Dim::Z}, Shape{6}, sc_units::m, Values{7, 8, 9, 12, 13, 14});
  const auto flat = flatten(var.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 4}),
                            std::vector<Dim>{Dim::X, Dim::Y}, Dim::Z);
  EXPECT_EQ(flat, expected);
  EXPECT_NE(flat.data_handle(), var.data_handle()); // copy since noncontiguous
}

TEST(ShapeTest, flatten_bad_dim_order) {
  const auto var = cumsum(
      variable::ones({{Dim::X, 6}, {Dim::Y, 4}}, sc_units::m, dtype<double>));
  EXPECT_THROW_DISCARD(flatten(var, std::vector<Dim>{Dim::Y, Dim::X}, Dim::Z),
                       except::DimensionError);
}

TEST(ShapeTest, flatten_non_contiguous) {
  Dimensions xy = {{Dim::X, 2}, {Dim::Y, 3}, {Dim::Z, 4}};
  const auto var = makeVariable<double>(xy);
  EXPECT_THROW_MSG_DISCARD(
      flatten(var, std::vector<Dim>{Dim::X, Dim::Z}, Dim::Time),
      except::DimensionError,
      "Can only flatten a contiguous set of dimensions in the correct order");
}

TEST(ShapeTest, flatten_0d) {
  const auto var = makeVariable<double>(Values{1});
  const auto expected = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1});
  const auto flat = flatten(var, std::vector<Dim>{}, Dim::X);
  EXPECT_EQ(flat, expected);
  EXPECT_EQ(flat.strides()[0], 1);
}

TEST(ShapeTest, round_trip) {
  const auto var = cumsum(
      variable::ones({{Dim::X, 6}, {Dim::Y, 4}}, sc_units::m, dtype<double>));
  const auto reshaped = fold(var, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}});
  EXPECT_EQ(flatten(reshaped, std::vector<Dim>{Dim::Row, Dim::Time}, Dim::X),
            var);
}

TEST(TransposeTest, make_transposed_2d) {
  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2},
                                  Values{1, 2, 3, 4, 5, 6},
                                  Variances{11, 12, 13, 14, 15, 16});

  const auto constVar = copy(var);

  auto ref = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                  Values{1, 3, 5, 2, 4, 6},
                                  Variances{11, 13, 15, 12, 14, 16});
  EXPECT_EQ(transpose(var, std::vector<Dim>{Dim::Y, Dim::X}), ref);
  EXPECT_EQ(transpose(constVar, std::vector<Dim>{Dim::Y, Dim::X}), ref);

  EXPECT_THROW_DISCARD(transpose(constVar, std::vector<Dim>{Dim::Y, Dim::Z}),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(transpose(constVar, std::vector<Dim>{Dim::Y}),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(transpose(constVar, std::vector<Dim>{Dim::Y, Dim::Z}),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(transpose(var, std::vector<Dim>{Dim::Z}),
                       except::DimensionError);
}

TEST(TransposeTest, make_transposed_multiple_d) {
  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y, Dim::Z}, Shape{3, 2, 1},
                                  Values{1, 2, 3, 4, 5, 6},
                                  Variances{11, 12, 13, 14, 15, 16});

  const auto constVar = copy(var);

  auto ref = makeVariable<double>(Dims{Dim::Y, Dim::Z, Dim::X}, Shape{2, 1, 3},
                                  Values{1, 3, 5, 2, 4, 6},
                                  Variances{11, 13, 15, 12, 14, 16});
  EXPECT_EQ(transpose(var, std::vector<Dim>{Dim::Y, Dim::Z, Dim::X}), ref);
  EXPECT_EQ(transpose(constVar, std::vector<Dim>{Dim::Y, Dim::Z, Dim::X}), ref);

  EXPECT_THROW_DISCARD(transpose(constVar, std::vector<Dim>{Dim::Y, Dim::Z}),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(transpose(constVar, std::vector<Dim>{Dim::Y}),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(transpose(var, std::vector<Dim>{Dim::Y, Dim::Z}),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(transpose(var, std::vector<Dim>{Dim::Z}),
                       except::DimensionError);
}

TEST(TransposeTest, reverse) {
  Variable var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2},
                                      Values{1, 2, 3, 4, 5, 6},
                                      Variances{11, 12, 13, 14, 15, 16});
  const Variable constVar = makeVariable<double>(
      Dims{Dim::X, Dim::Y}, Shape{3, 2}, Values{1, 2, 3, 4, 5, 6},
      Variances{11, 12, 13, 14, 15, 16});
  auto ref = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                  Values{1, 3, 5, 2, 4, 6},
                                  Variances{11, 13, 15, 12, 14, 16});
  auto tvar = transpose(var);
  auto tconstVar = transpose(constVar);
  EXPECT_EQ(tvar, ref);
  EXPECT_EQ(tconstVar, ref);
  auto v = transpose(makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2},
                                          Values{1, 2, 3, 4, 5, 6},
                                          Variances{11, 12, 13, 14, 15, 16}));
  EXPECT_EQ(v, ref);

  EXPECT_EQ(transpose(transpose(var)), var);
  EXPECT_EQ(transpose(transpose(constVar)), var);

  Variable dummy = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 1},
                                        Values{0, 0}, Variances{1, 1});
  EXPECT_NO_THROW(copy(dummy, tvar.slice({Dim::X, 0, 1})));
}
