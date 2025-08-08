// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/creation.h"
#include "test_macros.h"
#include "test_variables.h"

using namespace scipp;
using namespace scipp::variable;

TEST(CreationTest, empty) {
  const auto dims = Dimensions(Dim::X, 2);
  const auto var1 = variable::empty(dims, sc_units::m, dtype<double>, true);
  EXPECT_EQ(var1.dims(), dims);
  EXPECT_EQ(var1.unit(), sc_units::m);
  EXPECT_EQ(var1.dtype(), dtype<double>);
  EXPECT_EQ(var1.has_variances(), true);
  const auto var2 = variable::empty(dims, sc_units::s, dtype<int32_t>);
  EXPECT_EQ(var2.dims(), dims);
  EXPECT_EQ(var2.unit(), sc_units::s);
  EXPECT_EQ(var2.dtype(), dtype<int32_t>);
  EXPECT_EQ(var2.has_variances(), false);
}

TEST(CreationTest, ones) {
  const auto dims = Dimensions(Dim::X, 2);
  EXPECT_EQ(
      variable::ones(dims, sc_units::m, dtype<double>, true),
      makeVariable<double>(dims, sc_units::m, Values{1, 1}, Variances{1, 1}));
  EXPECT_EQ(variable::ones(dims, sc_units::s, dtype<int32_t>),
            makeVariable<int32_t>(dims, sc_units::s, Values{1, 1}));
  // Not a broadcast of a scalar
  EXPECT_FALSE(
      variable::ones(dims, sc_units::m, dtype<double>, true).is_readonly());
}

TEST_P(DenseVariablesTest, empty_like_fail_if_sizes) {
  const auto var = GetParam();
  EXPECT_THROW_DISCARD(
      empty_like(var, {}, makeVariable<scipp::index>(Values{12})),
      except::TypeError);
}

TEST_P(DenseVariablesTest, empty_like_default_shape) {
  const auto var = GetParam();
  const auto empty = empty_like(var);
  EXPECT_EQ(empty.dtype(), var.dtype());
  EXPECT_EQ(empty.dims(), var.dims());
  EXPECT_EQ(empty.unit(), var.unit());
  EXPECT_EQ(empty.has_variances(), var.has_variances());
}

TEST_P(DenseVariablesTest, empty_like_slice_default_shape) {
  const auto var = GetParam();
  if (var.dims().contains(Dim::X) && var.dims()[Dim::X] > 0) {
    const auto empty = empty_like(var.slice({Dim::X, 0}));
    EXPECT_EQ(empty.dtype(), var.dtype());
    EXPECT_EQ(empty.dims(), var.slice({Dim::X, 0}).dims());
    EXPECT_EQ(empty.unit(), var.unit());
    EXPECT_EQ(empty.has_variances(), var.has_variances());
  }
}

TEST_P(DenseVariablesTest, empty_like) {
  const auto var = GetParam();
  const Dimensions dims(Dim::X, 4);
  const auto empty = empty_like(var, dims);
  EXPECT_EQ(empty.dtype(), var.dtype());
  EXPECT_EQ(empty.dims(), dims);
  EXPECT_EQ(empty.unit(), var.unit());
  EXPECT_EQ(empty.has_variances(), var.has_variances());
}

TEST(CreationTest, special_like_double) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                        Values{1, 2}, Variances{3, 4});
  EXPECT_EQ(special_like(var, FillValue::Default),
            makeVariable<double>(var.dims(), var.unit(), Values{0, 0},
                                 Variances{0, 0}));
  EXPECT_EQ(special_like(var, FillValue::ZeroNotBool),
            makeVariable<double>(var.dims(), var.unit(), Values{0, 0},
                                 Variances{0, 0}));
  EXPECT_EQ(special_like(var, FillValue::True),
            makeVariable<bool>(var.dims(), var.unit(), Values{true, true}));
  EXPECT_EQ(special_like(var, FillValue::False),
            makeVariable<bool>(var.dims(), var.unit(), Values{false, false}));
  EXPECT_EQ(special_like(var, FillValue::Max),
            makeVariable<double>(var.dims(), var.unit(),
                                 Values{std::numeric_limits<double>::max(),
                                        std::numeric_limits<double>::max()},
                                 Variances{0, 0}));
  EXPECT_EQ(special_like(var, FillValue::Lowest),
            makeVariable<double>(var.dims(), var.unit(),
                                 Values{std::numeric_limits<double>::lowest(),
                                        std::numeric_limits<double>::lowest()},
                                 Variances{0, 0}));
}

TEST(CreationTest, special_like_int) {
  const auto var =
      makeVariable<int64_t>(Dims{Dim::X}, Shape{2}, sc_units::m, Values{1, 2});
  EXPECT_EQ(special_like(var, FillValue::Default),
            makeVariable<int64_t>(var.dims(), var.unit(), Values{0, 0}));
  EXPECT_EQ(special_like(var, FillValue::ZeroNotBool),
            makeVariable<int64_t>(var.dims(), var.unit(), Values{0, 0}));
  EXPECT_EQ(special_like(var, FillValue::True),
            makeVariable<bool>(var.dims(), var.unit(), Values{true, true}));
  EXPECT_EQ(special_like(var, FillValue::False),
            makeVariable<bool>(var.dims(), var.unit(), Values{false, false}));
  EXPECT_EQ(special_like(var, FillValue::Max),
            makeVariable<int64_t>(var.dims(), var.unit(),
                                  Values{std::numeric_limits<int64_t>::max(),
                                         std::numeric_limits<int64_t>::max()}));
  EXPECT_EQ(
      special_like(var, FillValue::Lowest),
      makeVariable<int64_t>(var.dims(), var.unit(),
                            Values{std::numeric_limits<int64_t>::lowest(),
                                   std::numeric_limits<int64_t>::lowest()}));
}

TEST(CreationTest, special_like_bool) {
  const auto var = makeVariable<bool>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                      Values{true, false});
  EXPECT_EQ(special_like(var, FillValue::Default),
            makeVariable<bool>(var.dims(), var.unit(), Values{false, false}));
  EXPECT_EQ(special_like(var, FillValue::ZeroNotBool),
            makeVariable<int64_t>(var.dims(), var.unit(), Values{0, 0}));
  EXPECT_EQ(special_like(var, FillValue::Max),
            makeVariable<bool>(var.dims(), var.unit(),
                               Values{std::numeric_limits<bool>::max(),
                                      std::numeric_limits<bool>::max()}));
  EXPECT_EQ(special_like(var, FillValue::Lowest),
            makeVariable<bool>(var.dims(), var.unit(),
                               Values{std::numeric_limits<bool>::lowest(),
                                      std::numeric_limits<bool>::lowest()}));
}

TEST(CreationTest, special_like_time_point) {
  using core::time_point;
  const auto var =
      makeVariable<time_point>(sc_units::ns, Values{time_point(1)});
  EXPECT_EQ(special_like(var, FillValue::Default),
            makeVariable<time_point>(sc_units::ns, Values{time_point(0)}));
  EXPECT_EQ(special_like(var, FillValue::ZeroNotBool),
            makeVariable<time_point>(sc_units::ns, Values{time_point(0)}));
  EXPECT_EQ(special_like(var, FillValue::True),
            makeVariable<bool>(sc_units::ns, Values{true}));
  EXPECT_EQ(special_like(var, FillValue::False),
            makeVariable<bool>(sc_units::ns, Values{false}));
  EXPECT_EQ(special_like(var, FillValue::Max),
            makeVariable<time_point>(
                sc_units::ns,
                Values{time_point(std::numeric_limits<int64_t>::max())}));
  EXPECT_EQ(special_like(var, FillValue::Lowest),
            makeVariable<time_point>(
                sc_units::ns,
                Values{time_point(std::numeric_limits<int64_t>::lowest())}));
}
