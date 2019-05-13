// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <type_traits>

#include "dataset.h"
#include "dimensions.h"
#include "except.h"

using namespace scipp::core;

TEST(DimensionMismatchError, what) {
  Dimensions dims{{Dim::X, 1}, {Dim::Y, 2}};
  except::DimensionMismatchError error(dims, Dimensions{});
  EXPECT_EQ(
      error.what(),
      std::string("Expected dimensions {{Dim::X, 1}, {Dim::Y, 2}}, got {}."));
}

TEST(DimensionNotFoundError, what) {
  Dimensions dims{{Dim::X, 1}, {Dim::Y, 2}};
  except::DimensionNotFoundError error(dims, Dim::Z);
  EXPECT_EQ(error.what(), std::string("Expected dimension to be in {{Dim::X, "
                                      "1}, {Dim::Y, 2}}, got Dim::Z."));
}

TEST(DimensionLengthError, what) {
  Dimensions dims{{Dim::X, 1}, {Dim::Y, 2}};
  except::DimensionLengthError error(dims, Dim::Y, 3);
  EXPECT_EQ(error.what(),
            std::string("Expected dimension to be in {{Dim::X, 1}, {Dim::Y, "
                        "2}}, got Dim::Y with mismatching length 3."));
}

TEST(StringFormattingTest, to_string_Dataset) {
  Dataset a;
  a.setData("a", makeVariable<double>({}));
  a.setData("b", makeVariable<double>({}));
  // Create new dataset with same variables but different order
  Dataset b;
  b.setData("b", makeVariable<double>({}));
  b.setData("a", makeVariable<double>({}));
  // string representations should be the same
  EXPECT_EQ(to_string(a), to_string(b));
}
