#include <gtest/gtest.h>
#include <vector>

#include "fix_typed_test_suite_warnings.h"
#include "test_macros.h"

#include "scipp/variable/special_values.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

TEST(VariableTest, nan_to_num_throws_when_input_and_replace_types_differ) {
  auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)});
  // Replacement type not same as input
  const auto replacement_value = makeVariable<int>(Values{-1});
  EXPECT_THROW(auto replaced = nan_to_num(a, replacement_value),
               except::TypeError);
}

TEST(VariableTest, nan_to_num) {
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

TEST(VariableTest, positive_inf_to_num) {
  auto a = makeVariable<double>(
      Dims{Dim::X}, Shape{3}, Values{1.0, double(INFINITY), double(-INFINITY)});
  auto replacement_value = makeVariable<double>(Values{-1});
  Variable b = pos_inf_to_num(a, replacement_value);
  auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{3},
      Values{1.0, replacement_value.value<double>(), double(-INFINITY)});
  EXPECT_EQ(b, expected);
}

TEST(VariableTest, negative_inf_to_num) {
  auto a = makeVariable<double>(
      Dims{Dim::X}, Shape{3}, Values{1.0, double(INFINITY), double(-INFINITY)});
  auto replacement_value = makeVariable<double>(Values{-1});
  Variable b = neg_inf_to_num(a, replacement_value);
  auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{3},
      Values{1.0, double(INFINITY), replacement_value.value<double>()});
  EXPECT_EQ(b, expected);
}

TEST(VariableTest,
     nan_to_num_with_variance_throws_if_replacement_has_no_variance) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                Values{1.0, double(NAN)}, Variances{0.1, 0.2});

  const auto replacement_value = makeVariable<double>(Values{-1});
  EXPECT_THROW(auto replaced = nan_to_num(a, replacement_value),
               except::VariancesError);
}

TEST(VariableTest, nan_to_num_with_variance_and_variance_on_replacement) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                Values{1.0, double(NAN)}, Variances{0.1, 0.2});

  const auto replacement = makeVariable<double>(Values{-1}, Variances{0.1});
  Variable b = nan_to_num(a, replacement);
  auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{1.0, replacement.value<double>()},
      Variances{0.1, replacement.variance<double>()});
  EXPECT_EQ(b, expected);
}

TEST(VariableTest,
     nan_to_num_inplace_throws_when_input_and_replace_types_differ) {
  auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)});
  // Replacement type not same as input
  const auto replacement_value = makeVariable<int>(Values{-1});
  EXPECT_THROW(nan_to_num(a, replacement_value, a), except::TypeError);
}

TEST(VariableTest,
     nan_to_num_inplace_throws_when_input_and_output_types_differ) {
  auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)});
  // Output type not same as input.
  auto out = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 1.0});
  const auto replacement_value = makeVariable<double>(Values{-1});
  EXPECT_THROW(nan_to_num(a, replacement_value, out), except::TypeError);
}

TEST(VariableTest, nan_to_num_inplace) {
  auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)});
  const auto replacement_value = makeVariable<double>(Values{-1});
  VariableView b = nan_to_num(a, replacement_value, a);
  auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{1.0, replacement_value.value<double>()});
  EXPECT_EQ(b, expected);
  EXPECT_EQ(a, expected);
}

TEST(VariableTest,
     nan_to_num_inplace_with_variance_throws_if_replacement_has_no_variance) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{3},
                                Values{1.0, double(NAN), 3.0},
                                Variances{0.1, 0.2, 0.3});
  const auto replacement_value = makeVariable<double>(Values{-1});
  EXPECT_THROW(nan_to_num(a, replacement_value, a), except::VariancesError);
}

TEST(VariableTest, nan_to_num_inplace_out_has_no_variances) {
  auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)});
  const auto replacement_value = makeVariable<double>(Values{-1});

  auto out = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{1.0, double(NAN)}, Variances{0.1, 0.2});

  EXPECT_THROW(nan_to_num(a, replacement_value, out), except::VariancesError);
}

TEST(VariableTest,
     nan_to_num_inplace_with_variance_and_variance_on_replacement) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                Values{1.0, double(NAN)}, Variances{0.1, 0.2});
  const auto replacement_value =
      makeVariable<double>(Values{-1}, Variances{0.1});
  VariableView b = nan_to_num(a, replacement_value, a);
  auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{1.0, replacement_value.value<double>()},
      Variances{0.1, replacement_value.variance<double>()});
  EXPECT_EQ(b, expected);
  EXPECT_EQ(a, expected);
}
