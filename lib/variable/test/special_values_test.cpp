#include <gtest/gtest.h>
#include <vector>

#include "fix_typed_test_suite_warnings.h"
#include "test_macros.h"

#include "scipp/core/element/special_values.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

template <typename T> constexpr auto values_for_special_value_tests() {
  return std::vector{static_cast<T>(0.0),
                     static_cast<T>(-1.23),
                     static_cast<T>(3e4),
                     std::numeric_limits<T>::quiet_NaN(),
                     std::numeric_limits<T>::signaling_NaN(),
                     std::numeric_limits<T>::infinity(),
                     -std::numeric_limits<T>::infinity()};
}

template <typename T>
class VariableSpecialValueTest : public ::testing::Test {};
using FloatTypes = ::testing::Types<double, float>;
TYPED_TEST_SUITE(VariableSpecialValueTest, FloatTypes);

TYPED_TEST(VariableSpecialValueTest, isnan) {
  for (TypeParam x : values_for_special_value_tests<TypeParam>()) {
    EXPECT_EQ(variable::isnan(x * sc_units::m),
              element::isnan(x) * sc_units::none);
  }
}

TYPED_TEST(VariableSpecialValueTest, isinf) {
  for (TypeParam x : values_for_special_value_tests<TypeParam>()) {
    EXPECT_EQ(variable::isinf(x * sc_units::m),
              element::isinf(x) * sc_units::none);
  }
}

TYPED_TEST(VariableSpecialValueTest, isfinite) {
  for (TypeParam x : values_for_special_value_tests<TypeParam>()) {
    EXPECT_EQ(variable::isfinite(x * sc_units::m),
              element::isfinite(x) * sc_units::none);
  }
  EXPECT_EQ(variable::isfinite(1 * sc_units::dimensionless),
            element::isfinite(1) * sc_units::none);
}

TYPED_TEST(VariableSpecialValueTest, isposinf) {
  for (TypeParam x : values_for_special_value_tests<TypeParam>()) {
    EXPECT_EQ(variable::isposinf(x * sc_units::m),
              element::isposinf(x) * sc_units::none);
  }
}

TYPED_TEST(VariableSpecialValueTest, isneginf) {
  for (TypeParam x : values_for_special_value_tests<TypeParam>()) {
    EXPECT_EQ(variable::isneginf(x * sc_units::m),
              element::isneginf(x) * sc_units::none);
  }
}

template <typename Op> void check_no_out_variances(Op op) {
  const auto var = makeVariable<double>(Dimensions{Dim::Z, 2}, sc_units::m,
                                        Values{1.0, 2.0}, Variances{1.0, 2.0});
  const Variable applied = op(var);
  EXPECT_FALSE(applied.has_variances());
  const Variable applied_on_values = op(values(var));
  EXPECT_EQ(applied, applied_on_values);
}

TEST(VariableSpecialValueTest, isfinite_no_out_variances) {
  using variable::isfinite;
  check_no_out_variances([](const auto &x) { return isfinite(x); });
}

TEST(VariableSpecialValueTest, isnan_no_out_variances) {
  using variable::isnan;
  check_no_out_variances([](const auto &x) { return isnan(x); });
}

TEST(VariableSpecialValueTest, isinf_no_out_variances) {
  using variable::isinf;
  check_no_out_variances([](const auto &x) { return isinf(x); });
}

TEST(VariableSpecialValueTest, isneginf_no_out_variances) {
  using variable::isneginf;
  check_no_out_variances([](const auto &x) { return isneginf(x); });
}
TEST(VariableSpecialValueTest, isposinf_no_out_variances) {
  using variable::isposinf;
  check_no_out_variances([](const auto &x) { return isposinf(x); });
}

TEST(VariableSpecialValueTest,
     nan_to_num_throws_when_input_and_replace_types_differ) {
  auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)});
  // Replacement type not same as input
  const auto replacement_value = makeVariable<int>(Values{-1});
  EXPECT_THROW(auto replaced = nan_to_num(a, replacement_value),
               except::TypeError);
}

TEST(VariableSpecialValueTest, nan_to_num) {
  auto a = makeVariable<double>(
      Dims{Dim::X}, Shape{4},
      Values{1.0, double(NAN), double(INFINITY), double(-INFINITY)});
  auto replacement_value = makeVariable<double>(Values{-1});
  Variable b = nan_to_num(a, replacement_value);
  auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{4},
                           Values{1.0, replacement_value.value<double>(),
                                  double(INFINITY), double(-INFINITY)});
  EXPECT_EQ(b, expected);
}

TEST(VariableSpecialValueTest, positive_inf_to_num) {
  auto a = makeVariable<double>(
      Dims{Dim::X}, Shape{3}, Values{1.0, double(INFINITY), double(-INFINITY)});
  auto replacement_value = makeVariable<double>(Values{-1});
  Variable b = pos_inf_to_num(a, replacement_value);
  auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{3},
      Values{1.0, replacement_value.value<double>(), double(-INFINITY)});
  EXPECT_EQ(b, expected);
}

TEST(VariableSpecialValueTest, negative_inf_to_num) {
  auto a = makeVariable<double>(
      Dims{Dim::X}, Shape{3}, Values{1.0, double(INFINITY), double(-INFINITY)});
  auto replacement_value = makeVariable<double>(Values{-1});
  Variable b = neg_inf_to_num(a, replacement_value);
  auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{3},
      Values{1.0, double(INFINITY), replacement_value.value<double>()});
  EXPECT_EQ(b, expected);
}

TEST(VariableSpecialValueTest,
     nan_to_num_with_variance_throws_if_replacement_has_no_variance) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                Values{1.0, double(NAN)}, Variances{0.1, 0.2});

  const auto replacement_value = makeVariable<double>(Values{-1});
  EXPECT_THROW(auto replaced = nan_to_num(a, replacement_value),
               except::VariancesError);
}

TEST(VariableSpecialValueTest,
     nan_to_num_with_variance_and_variance_on_replacement) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                Values{1.0, double(NAN)}, Variances{0.1, 0.2});

  const auto replacement = makeVariable<double>(Values{-1}, Variances{0.1});
  Variable b = nan_to_num(a, replacement);
  auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{1.0, replacement.value<double>()},
      Variances{0.1, replacement.variance<double>()});
  EXPECT_EQ(b, expected);
}

TEST(VariableSpecialValueTest,
     nan_to_num_inplace_throws_when_input_and_replace_types_differ) {
  auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)});
  // Replacement type not same as input
  const auto replacement_value = makeVariable<int>(Values{-1});
  EXPECT_THROW(nan_to_num(a, replacement_value, a), except::TypeError);
}

TEST(VariableSpecialValueTest,
     nan_to_num_inplace_throws_when_input_and_output_types_differ) {
  auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)});
  // Output type not same as input.
  auto out = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 1.0});
  const auto replacement_value = makeVariable<double>(Values{-1});
  EXPECT_THROW(nan_to_num(a, replacement_value, out), except::TypeError);
}

TEST(VariableSpecialValueTest, nan_to_num_inplace) {
  auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)});
  const auto replacement_value = makeVariable<double>(Values{-1});
  auto &b = nan_to_num(a, replacement_value, a);
  auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{1.0, replacement_value.value<double>()});
  EXPECT_EQ(b, expected);
  EXPECT_EQ(a, expected);
}

TEST(VariableSpecialValueTest,
     nan_to_num_inplace_with_variance_throws_if_replacement_has_no_variance) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{3},
                                Values{1.0, double(NAN), 3.0},
                                Variances{0.1, 0.2, 0.3});
  const auto replacement_value = makeVariable<double>(Values{-1});
  EXPECT_THROW(nan_to_num(a, replacement_value, a), except::VariancesError);
}

TEST(VariableSpecialValueTest, nan_to_num_inplace_out_has_no_variances) {
  auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)});
  const auto replacement_value = makeVariable<double>(Values{-1});

  auto out = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)}, Variances{0.1, 0.2});

  EXPECT_THROW(nan_to_num(a, replacement_value, out), except::VariancesError);
}

TEST(VariableSpecialValueTest,
     nan_to_num_inplace_with_variance_and_variance_on_replacement) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                Values{1.0, double(NAN)}, Variances{0.1, 0.2});
  const auto replacement_value =
      makeVariable<double>(Values{-1}, Variances{0.1});
  auto &b = nan_to_num(a, replacement_value, a);
  auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{1.0, replacement_value.value<double>()},
      Variances{0.1, replacement_value.variance<double>()});
  EXPECT_EQ(b, expected);
  EXPECT_EQ(a, expected);
}

TEST(VariableSpecialValueTest, isfinite_on_vector) {
  auto vec = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{2},
      Values{Eigen::Vector3d{1, 2, 4},
             Eigen::Vector3d{1, double(INFINITY), 4}});
  auto expected =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false});

  EXPECT_EQ(variable::isfinite(vec), expected);
}

TEST(VariableSpecialValueTest, isfinite_with_variance) {
  auto vec = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{double(NAN), 7.0}, Variances{1.0, 1.0});

  EXPECT_EQ(variable::isfinite(vec),
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true}));
}
