// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/constants.h"
#include "scipp/variable/trigonometry.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::units;

TEST(VariableTrigonometryTest, sin_rad) {
  const auto var = makeVariable<double>(Values{pi<double>}, Unit{units::rad});
  EXPECT_EQ(sin(var), makeVariable<double>(Values{std::sin(pi<double>)}));
}

TEST(VariableTrigonometryTest, sin_deg) {
  const auto var = makeVariable<double>(Values{180.0}, Unit{units::deg});
  EXPECT_EQ(sin(var), makeVariable<double>(Values{std::sin(pi<double>)}));
}

TEST(Variable, sin_move_rad) {
  auto var = makeVariable<double>(Values{pi<double>}, Unit{units::rad});
  const auto ptr = var.values<double>().data();
  auto out = sin(std::move(var));
  EXPECT_EQ(out, makeVariable<double>(Values{std::sin(pi<double>)}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, sin_move_deg) {
  auto var = makeVariable<double>(Values{180.0}, Unit{units::deg});
  const auto ptr = var.values<double>().data();
  auto out = sin(std::move(var));
  EXPECT_EQ(out, makeVariable<double>(Values{std::sin(pi<double>)}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, sin_out_arg_rad) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{pi<double>, 0.0},
                                Unit{units::rad});
  auto out = makeVariable<double>(Values{0.0});
  auto view = sin(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(out, makeVariable<double>(Values{std::sin(pi<double>)}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(Variable, sin_out_arg_deg) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{180.0, 0.0},
                                Unit{units::deg});
  auto out = makeVariable<double>(Values{0.0});
  auto view = sin(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(out, makeVariable<double>(Values{std::sin(pi<double>)}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(VariableTrigonometryTest, cos_rad) {
  const auto var = makeVariable<double>(Values{pi<double>}, Unit{units::rad});
  EXPECT_EQ(cos(var), makeVariable<double>(Values{std::cos(pi<double>)}));
}

TEST(VariableTrigonometryTest, cos_deg) {
  const auto var = makeVariable<double>(Values{180.0}, Unit{units::deg});
  EXPECT_EQ(cos(var), makeVariable<double>(Values{std::cos(pi<double>)}));
}

TEST(Variable, cos_move_rad) {
  auto var = makeVariable<double>(Values{pi<double>}, Unit{units::rad});
  const auto ptr = var.values<double>().data();
  auto out = cos(std::move(var));
  EXPECT_EQ(out, makeVariable<double>(Values{std::cos(pi<double>)}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, cos_move_deg) {
  auto var = makeVariable<double>(Values{180.0}, Unit{units::deg});
  const auto ptr = var.values<double>().data();
  auto out = cos(std::move(var));
  EXPECT_EQ(out, makeVariable<double>(Values{std::cos(pi<double>)}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, cos_out_arg_rad) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{pi<double>, 0.0},
                                Unit{units::rad});
  auto out = makeVariable<double>(Values{0.0});
  auto view = cos(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(out, makeVariable<double>(Values{std::cos(pi<double>)}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(Variable, cos_out_arg_deg) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{180.0, 0.0},
                                Unit{units::deg});
  auto out = makeVariable<double>(Values{0.0});
  auto view = cos(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(out, makeVariable<double>(Values{std::cos(pi<double>)}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(VariableTrigonometryTest, tan_rad) {
  const auto var = makeVariable<double>(Values{pi<double>}, Unit{units::rad});
  EXPECT_EQ(tan(var), makeVariable<double>(Values{std::tan(pi<double>)}));
}

TEST(VariableTrigonometryTest, tan_deg) {
  const auto var = makeVariable<double>(Values{180.0}, Unit{units::deg});
  EXPECT_EQ(tan(var), makeVariable<double>(Values{std::tan(pi<double>)}));
}

TEST(Variable, tan_move_rad) {
  auto var = makeVariable<double>(Values{pi<double>}, Unit{units::rad});
  const auto ptr = var.values<double>().data();
  auto out = tan(std::move(var));
  EXPECT_EQ(out, makeVariable<double>(Values{std::tan(pi<double>)}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, tan_move_deg) {
  auto var = makeVariable<double>(Values{180.0}, Unit{units::deg});
  const auto ptr = var.values<double>().data();
  auto out = tan(std::move(var));
  EXPECT_EQ(out, makeVariable<double>(Values{std::tan(pi<double>)}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, tan_out_arg_rad) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{pi<double>, 0.0},
                                Unit{units::rad});
  auto out = makeVariable<double>(Values{0.0});
  auto view = tan(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(out, makeVariable<double>(Values{std::tan(pi<double>)}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(Variable, tan_out_arg_deg) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{180.0, 0.0},
                                Unit{units::deg});
  auto out = makeVariable<double>(Values{0.0});
  auto view = tan(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(out, makeVariable<double>(Values{std::tan(pi<double>)}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(VariableTrigonometryTest, asin) {
  const auto var = makeVariable<double>(Values{1.0});
  EXPECT_EQ(asin(var),
            makeVariable<double>(Values{std::asin(1.0)}, Unit{units::rad}));
}

TEST(Variable, asin_move) {
  auto var = makeVariable<double>(Values{1.0});
  const auto ptr = var.values<double>().data();
  auto out = asin(std::move(var));
  EXPECT_EQ(out,
            makeVariable<double>(Values{std::asin(1.0)}, Unit{units::rad}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, asin_out_arg) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 0.0});
  auto out = makeVariable<double>(Values{0.0});
  auto view = asin(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(out,
            makeVariable<double>(Values{std::asin(1.0)}, Unit{units::rad}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(VariableTrigonometryTest, acos) {
  const auto var = makeVariable<double>(Values{1.0});
  EXPECT_EQ(acos(var),
            makeVariable<double>(Values{std::acos(1.0)}, Unit{units::rad}));
}

TEST(Variable, acos_move) {
  auto var = makeVariable<double>(Values{1.0});
  const auto ptr = var.values<double>().data();
  auto out = acos(std::move(var));
  EXPECT_EQ(out,
            makeVariable<double>(Values{std::acos(1.0)}, Unit{units::rad}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, acos_out_arg) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 0.0});
  auto out = makeVariable<double>(Values{0.0});
  auto view = acos(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(out,
            makeVariable<double>(Values{std::acos(1.0)}, Unit{units::rad}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(VariableTrigonometryTest, atan) {
  const auto var = makeVariable<double>(Values{1.0});
  EXPECT_EQ(atan(var),
            makeVariable<double>(Values{std::atan(1.0)}, Unit{units::rad}));
}

TEST(Variable, atan_move) {
  auto var = makeVariable<double>(Values{1.0});
  const auto ptr = var.values<double>().data();
  auto out = atan(std::move(var));
  EXPECT_EQ(out,
            makeVariable<double>(Values{std::atan(1.0)}, Unit{units::rad}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, atan_out_arg) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 0.0});
  auto out = makeVariable<double>(Values{0.0});
  auto view = atan(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(out,
            makeVariable<double>(Values{std::atan(1.0)}, Unit{units::rad}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(VariableTrigonometryTest, atan2) {
  auto x = makeVariable<double>(units::m, Values{1.0});
  auto y = x;
  auto expected =
      makeVariable<double>(units::rad, Values{scipp::pi<double> / 4});
  EXPECT_EQ(atan2(y, x), expected);
}

TEST(VariableTrigonometryTest, atan2_out_arg) {
  auto x = makeVariable<double>(units::m, Values{1.0});
  auto y = x;
  auto expected =
      makeVariable<double>(units::rad, Values{scipp::pi<double> / 4});
  auto out = atan2(y, x, y);
  EXPECT_EQ(out, expected);
  EXPECT_EQ(y, expected);
}
