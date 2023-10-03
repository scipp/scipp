// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <type_traits>

#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(StringFormattingTest, to_string_Dataset) {
  Dataset a({{"a", makeVariable<double>(Values{double{}})},
             {"b", makeVariable<double>(Values{double{}})}});
  // Create new dataset with same variables but different order
  Dataset b({{"b", makeVariable<double>(Values{double{}})},
             {"a", makeVariable<double>(Values{double{}})}});
  // string representations should be the same
  EXPECT_EQ(to_string(a), to_string(b));
}

TEST(StringFormattingTest, to_string_MutableView) {
  Dataset a({},
            {
                {Dim::X,
                 makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3})},
                {Dim::Y,
                 makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1, 2, 3})},
                {Dim::Z,
                 makeVariable<double>(Dims{Dim::Z}, Shape{3}, Values{1, 2, 3})},
                {Dim("label_1"),
                 makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{21, 22, 23})},
                {Dim("label_2"),
                 makeVariable<int>(Dims{Dim::Y}, Shape{3}, Values{21, 22, 23})},
                {Dim("label_3"),
                 makeVariable<int>(Dims{Dim::Z}, Shape{3}, Values{21, 22, 23})},
            });
  EXPECT_NO_THROW(to_string(a.coords()));
}

TEST(StringFormattingTest, to_string_ConstView) {
  const Dataset a(
      {}, {
              {Dim::X,
               makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3})},
              {Dim::Y,
               makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1, 2, 3})},
              {Dim::Z,
               makeVariable<double>(Dims{Dim::Z}, Shape{3}, Values{1, 2, 3})},
              {Dim("label_1"),
               makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{21, 22, 23})},
              {Dim("label_2"),
               makeVariable<int>(Dims{Dim::Y}, Shape{3}, Values{21, 22, 23})},
              {Dim("label_3"),
               makeVariable<int>(Dims{Dim::Z}, Shape{3}, Values{21, 22, 23})},
          });
  EXPECT_NO_THROW(to_string(a.coords()));
}

TEST(ValidSliceTest, test_slice_range) {
  Dimensions dims{Dim::X, 3};
  EXPECT_NO_THROW(core::expect::validSlice(dims, Slice(Dim::X, 0)));
  EXPECT_NO_THROW(core::expect::validSlice(dims, Slice(Dim::X, 2)));
  EXPECT_NO_THROW(core::expect::validSlice(dims, Slice(Dim::X, 0, 3)));
  EXPECT_THROW(core::expect::validSlice(dims, Slice(Dim::X, 3)),
               except::SliceError);
  EXPECT_THROW(core::expect::validSlice(dims, Slice(Dim::X, -1)),
               except::SliceError);
  EXPECT_THROW(core::expect::validSlice(dims, Slice(Dim::X, 0, 4)),
               except::SliceError);
}

TEST(ValidSliceTest, test_dimension_contained) {
  Dimensions dims{{Dim::X, 3}, {Dim::Z, 3}};
  EXPECT_NO_THROW(core::expect::validSlice(dims, Slice(Dim::X, 0)));
  EXPECT_THROW(core::expect::validSlice(dims, Slice(Dim::Y, 0)),
               except::SliceError);
}
