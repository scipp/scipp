// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/eigen.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/util.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bin_array_model.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/element_array_model.h"
#include "scipp/variable/structure_array_model.h"

using namespace scipp;
using namespace scipp::dataset;

class BinnedVariableSizeOfTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 3};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 2}, std::pair{2, 4}});
  Variable buffer = makeVariable<double>(Dims{Dim::X}, Shape{4});
  Variable var = make_bins(indices, Dim::X, buffer);

  const scipp::index object_size =
      sizeof(Variable) + sizeof(variable::BinArrayModel<Variable>);
};

class BinnedDataArraySizeOfTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable data = makeVariable<double>(Dims{Dim::X}, Shape{4});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  Variable var = make_bins(indices, Dim::X, buffer);

  const scipp::index object_size =
      sizeof(Variable) + sizeof(variable::BinArrayModel<DataArray>);
};

class BinnedDatasetSizeOfTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable column = makeVariable<double>(Dims{Dim::X}, Shape{4});
  Dataset buffer;

  const scipp::index object_size =
      sizeof(Variable) + sizeof(variable::BinArrayModel<Dataset>);
};

TEST(SizeOf, variable) {
  const auto object_size =
      sizeof(Variable) + sizeof(variable::ElementArrayModel<double>);

  auto var = makeVariable<double>(Shape{4}, Dims{Dim::X});
  EXPECT_EQ(size_of(var, SizeofTag::ViewOnly),
            sizeof(double) * 4 + object_size);
  EXPECT_EQ(size_of(var, SizeofTag::Underlying),
            sizeof(double) * 4 + object_size);

  auto var_with_variance = makeVariable<double>(
      Shape{1, 2}, Dims{Dim::X, Dim::Y}, Values{3, 4}, Variances{1, 2});

  EXPECT_EQ(size_of(var_with_variance, SizeofTag::ViewOnly),
            sizeof(double) * 4 + object_size);
  EXPECT_EQ(size_of(var_with_variance, SizeofTag::Underlying),
            sizeof(double) * 4 + object_size);
}

TEST(SizeOf, sliced_variable) {
  const auto object_size =
      sizeof(Variable) + sizeof(variable::ElementArrayModel<Variable>);

  auto var = makeVariable<double>(Shape{4}, Dims{Dim::X});
  auto sliced_view = var.slice(Slice(Dim::X, 0, 2));
  EXPECT_EQ(size_of(sliced_view, SizeofTag::ViewOnly),
            2 * sizeof(double) + object_size);
  EXPECT_EQ(size_of(sliced_view, SizeofTag::Underlying),
            4 * sizeof(double) + object_size);
}

TEST(SizeOf, variable_of_vector3) {
  const auto object_size =
      sizeof(Variable) +
      sizeof(variable::StructureArrayModel<Eigen::Vector3d, double>) +
      sizeof(variable::ElementArrayModel<double>);

  auto var = makeVariable<Eigen::Vector3d>(Shape{1, 1}, Dims{Dim::X, Dim::Y});
  EXPECT_EQ(size_of(var, SizeofTag::ViewOnly),
            sizeof(Eigen::Vector3d) + object_size);
  EXPECT_EQ(size_of(var, SizeofTag::Underlying),
            sizeof(Eigen::Vector3d) + object_size);
}

namespace {
auto short_string_size([[maybe_unused]] const std::string &str) {
#ifdef _MSC_VER
  // MSVC does not use short string optimization.
  return sizeof(std::string) + str.size();
#else
  return sizeof(std::string);
#endif
}
} // namespace

TEST(SizeOf, variable_of_short_string) {
  const auto object_size =
      sizeof(Variable) + sizeof(variable::ElementArrayModel<std::string>);

  const std::string str = "short";
  const auto var =
      makeVariable<std::string>(Shape{1}, Dims{Dim::X}, Values{str});
  const auto expected_size = short_string_size(str) + object_size;
  EXPECT_EQ(size_of(var, SizeofTag::ViewOnly), expected_size);
  EXPECT_EQ(size_of(var, SizeofTag::Underlying), expected_size);
}

TEST(SizeOf, variable_of_long_string) {
  const auto object_size =
      sizeof(Variable) + sizeof(variable::ElementArrayModel<std::string>);

  const std::string str = "A rather long string that is hopefully on the heap";
  ASSERT_GT(str.size(), sizeof(std::string));
  const auto expected_size = sizeof(std::string) + str.size() + object_size;
  const auto var =
      makeVariable<std::string>(Shape{1}, Dims{Dim::X}, Values{str});
  EXPECT_EQ(size_of(var, SizeofTag::ViewOnly), expected_size);
  EXPECT_EQ(size_of(var, SizeofTag::Underlying), expected_size);
}

TEST(SizeOf, variable_of_three_strings) {
  const auto object_size =
      sizeof(Variable) + sizeof(variable::ElementArrayModel<std::string>);

  const std::string str0 = "A rather long string that is hopefully on the heap";
  const std::string str1;
  const std::string str2 = "short";
  const auto expected_size = 2 * sizeof(std::string) + str0.size() +
                             short_string_size(str2) + object_size;
  const auto var = makeVariable<std::string>(Shape{3}, Dims{Dim::X},
                                             Values{str0, str1, str2});
  EXPECT_EQ(size_of(var, SizeofTag::ViewOnly), expected_size);
  EXPECT_EQ(size_of(var, SizeofTag::Underlying), expected_size);
}

TEST(SizeOf, variable_of_three_strings_slice) {
  const auto object_size =
      sizeof(Variable) + sizeof(variable::ElementArrayModel<std::string>);

  const std::string str0 = "A rather long string that is hopefully on the heap";
  const std::string str1;
  const std::string str2 = "short";
  const auto full = makeVariable<std::string>(Shape{3}, Dims{Dim::X},
                                              Values{str0, str1, str2});
  const auto var1 = full.slice({Dim::X, 0, 2});
  const auto full_size = 2 * sizeof(std::string) + str0.size() +
                         short_string_size(str2) + object_size;
  EXPECT_EQ(size_of(var1, SizeofTag::ViewOnly),
            2 * sizeof(std::string) + str0.size() + object_size);
  EXPECT_EQ(size_of(var1, SizeofTag::Underlying), full_size);
  const auto var2 = full.slice({Dim::X, 1, 3});
  EXPECT_EQ(size_of(var2, SizeofTag::ViewOnly),
            sizeof(std::string) + short_string_size(str2) + object_size);
  EXPECT_EQ(size_of(var2, SizeofTag::Underlying), full_size);
  const auto var3 = full.slice({Dim::X, 1, 1});
  EXPECT_EQ(size_of(var3, SizeofTag::ViewOnly), object_size);
  EXPECT_EQ(size_of(var3, SizeofTag::Underlying), full_size);
}

TEST(SizeOf, variable_of_variable) {
  const auto object_size =
      sizeof(Variable) + sizeof(variable::ElementArrayModel<Variable>);

  const auto inner =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto outer = makeVariable<Variable>(Shape{}, Values{inner});
  EXPECT_EQ(size_of(outer, SizeofTag::ViewOnly),
            size_of(inner, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(outer, SizeofTag::Underlying),
            size_of(inner, SizeofTag::Underlying) + object_size);
}

TEST(SizeOf, slice_of_variable_of_variables) {
  const auto object_size =
      sizeof(Variable) + sizeof(variable::ElementArrayModel<Variable>);

  const auto inner1 =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto inner2 =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{4, 5});
  const auto outer =
      makeVariable<Variable>(Dims{Dim::Y}, Shape{2}, Values{inner1, inner2});
  const auto sliced = outer.slice({Dim::Y, 1, 2});
  EXPECT_EQ(size_of(sliced, SizeofTag::ViewOnly),
            size_of(inner2, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(sliced, SizeofTag::Underlying),
            size_of(outer, SizeofTag::Underlying));
}

TEST(SizeOf, variable_of_sliced_variables) {
  const auto object_size =
      sizeof(Variable) + sizeof(variable::ElementArrayModel<Variable>);

  const auto inner1 =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto sliced_inner1 = inner1.slice({Dim::X, 0, 2});
  const auto inner2 =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{4, 5});
  const auto outer = makeVariable<Variable>(Dims{Dim::Y}, Shape{2},
                                            Values{sliced_inner1, inner2});
  EXPECT_EQ(size_of(outer, SizeofTag::ViewOnly),
            size_of(sliced_inner1, SizeofTag::ViewOnly) +
                size_of(inner2, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(outer, SizeofTag::Underlying),
            size_of(inner1, SizeofTag::Underlying) +
                size_of(inner2, SizeofTag::Underlying) + object_size);
}

TEST(SizeOf, variable_of_data_array) {
  const auto object_size =
      sizeof(Variable) + sizeof(variable::ElementArrayModel<Variable>);

  const auto data = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0, 1});
  const auto coord =
      makeVariable<int64_t>(Dims{Dim::X}, Shape{3}, Values{0, 1, 2});
  const DataArray da(data, {{Dim::X, coord}});
  const auto outer = makeVariable<DataArray>(Shape{}, Values{da});
  EXPECT_EQ(size_of(outer, SizeofTag::ViewOnly),
            size_of(da, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(outer, SizeofTag::Underlying),
            size_of(da, SizeofTag::Underlying) + object_size);
}

TEST(SizeOf, data_array) {
  const auto object_size = sizeof(DataArray) + sizeof(dataset::Coords) +
                           sizeof(dataset::Attrs) + sizeof(dataset::Masks);

  const auto data = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0, 1});
  const auto coord =
      makeVariable<int64_t>(Dims{Dim::X}, Shape{3}, Values{0, 1, 2});
  const DataArray da(data, {{Dim::X, coord}});
  EXPECT_EQ(size_of(da, SizeofTag::ViewOnly),
            size_of(data, SizeofTag::ViewOnly) +
                size_of(coord, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(da, SizeofTag::Underlying),
            size_of(data, SizeofTag::Underlying) +
                size_of(coord, SizeofTag::Underlying) + object_size);
}

TEST_F(BinnedVariableSizeOfTest, full_variable) {
  const auto &[indices_, dim_, buffer_] = var.constituents<Variable>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(var, SizeofTag::ViewOnly),
            size_of(buffer_, SizeofTag::ViewOnly) +
                size_of(indices_, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(var, SizeofTag::Underlying),
            size_of(buffer_, SizeofTag::Underlying) +
                size_of(indices_, SizeofTag::Underlying) + object_size);
}

TEST_F(BinnedVariableSizeOfTest, sliced_variable) {
  auto slice = var.slice(Slice(Dim::Y, 0, 1));
  const auto &[indices_, dim_, buffer_] = slice.constituents<Variable>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(slice, SizeofTag::ViewOnly),
            size_of(buffer_, SizeofTag::ViewOnly) * 0.5 +
                size_of(indices_, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(slice, SizeofTag::Underlying),
            size_of(buffer_, SizeofTag::Underlying) +
                size_of(indices_, SizeofTag::Underlying) + object_size);
}

TEST_F(BinnedVariableSizeOfTest, empty_buffer) {
  Variable empty(var.slice(Slice(Dim::Y, 1)));
  const auto &[indices_, dim_, buffer_] = empty.constituents<Variable>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(empty, SizeofTag::ViewOnly),
            size_of(indices_, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(empty, SizeofTag::Underlying),
            size_of(buffer_, SizeofTag::Underlying) +
                size_of(indices_, SizeofTag::Underlying) + object_size);
}

TEST_F(BinnedDataArraySizeOfTest, full_variable) {
  const auto &[indices_, dim_, buffer_] = var.constituents<DataArray>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(var, SizeofTag::ViewOnly),
            size_of(buffer_, SizeofTag::ViewOnly) +
                size_of(indices_, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(var, SizeofTag::Underlying),
            size_of(buffer_, SizeofTag::Underlying) +
                size_of(indices_, SizeofTag::Underlying) + object_size);
}

TEST_F(BinnedDataArraySizeOfTest, sliced_variable) {
  auto slice = var.slice(Slice(Dim::Y, 0, 1));
  const auto &[indices_, dim_, buffer_] = slice.constituents<DataArray>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(slice, SizeofTag::ViewOnly),
            size_of(buffer_, SizeofTag::ViewOnly) * 0.5 +
                size_of(indices_, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(slice, SizeofTag::Underlying),
            size_of(buffer_, SizeofTag::Underlying) +
                size_of(indices_, SizeofTag::Underlying) + object_size);
}

TEST_F(BinnedDatasetSizeOfTest, full_variable) {
  buffer.setCoord(Dim::X, column);
  Variable var = make_bins(indices, Dim::X, buffer);
  const auto &[indices_, dim_, buffer_] = var.constituents<Dataset>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(var, SizeofTag::ViewOnly),
            size_of(buffer_, SizeofTag::ViewOnly) +
                size_of(indices_, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(var, SizeofTag::Underlying),
            size_of(buffer_, SizeofTag::Underlying) +
                size_of(indices_, SizeofTag::Underlying) + object_size);
}

TEST_F(BinnedDatasetSizeOfTest, sliced_variable) {
  buffer.setCoord(Dim::X, column);
  Variable var = make_bins(indices, Dim::X, buffer);
  auto slice = var.slice(Slice(Dim::Y, 0, 1));
  const auto &[indices_, dim_, buffer_] = slice.constituents<Dataset>();
  EXPECT_EQ(dim_, Dim::X);
  EXPECT_EQ(size_of(slice, SizeofTag::ViewOnly),
            size_of(buffer_, SizeofTag::ViewOnly) * 0.5 +
                size_of(indices_, SizeofTag::ViewOnly) + object_size);
  EXPECT_EQ(size_of(slice, SizeofTag::Underlying),
            size_of(buffer_, SizeofTag::Underlying) +
                size_of(indices_, SizeofTag::Underlying) + object_size);
}
