/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include <type_traits>

#include "tags.h"

TEST(Tags, tag_type) {
  EXPECT_TRUE((std::is_same<tag_type_t<Data::Value>, double>::value));
  EXPECT_TRUE((std::is_same<tag_type_t<Data::Any, double>, double>::value));
  EXPECT_TRUE((std::is_same<tag_type_t<Data::Any, float>, float>::value));
}
