/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include <type_traits>

#include "dimensions.h"
#include "except.h"

TEST(DimensionMismatchError, what) {
  Dimensions dims{{Dim::X, 1}, {Dim::Y, 2}};
  dataset::except::DimensionMismatchError error(dims, Dimensions{});
  EXPECT_EQ(
      error.what(),
      std::string("Expected dimensions {{Dim::X, 1}, {Dim::Y, 2}}, got {}."));
}

TEST(DimensionNotFoundError, what) {
  Dimensions dims{{Dim::X, 1}, {Dim::Y, 2}};
  dataset::except::DimensionNotFoundError error(dims, Dim::Z);
  EXPECT_EQ(error.what(), std::string("Expected dimension to be in {{Dim::X, "
                                      "1}, {Dim::Y, 2}}, got Dim::Z."));
}

TEST(DimensionLengthError, what) {
  Dimensions dims{{Dim::X, 1}, {Dim::Y, 2}};
  dataset::except::DimensionLengthError error(dims, Dim::Y, 3);
  EXPECT_EQ(error.what(),
            std::string("Expected dimension to be in {{Dim::X, 1}, {Dim::Y, "
                        "2}}, got Dim::Y with mismatching length 3."));
}
