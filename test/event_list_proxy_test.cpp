/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include <type_traits>

#include "event_list_proxy.h"

TEST(ConstEventListProxy, from_vectors) {
  std::vector<double> a{1.1, 2.2, 3.3};
  std::vector<int32_t> b{1, 2, 3};
  ConstEventListProxy proxy(a, b);
  EXPECT_EQ(std::get<0>(*proxy.begin()), 1.1);
  EXPECT_EQ(std::get<1>(*proxy.begin()), 1);
}

TEST(EventListProxy, from_vectors) {
  std::vector<double> a{1.1, 2.2, 3.3};
  std::vector<int32_t> b{1, 2, 3};
  EventListProxy proxy(a, b);
  EXPECT_EQ(std::get<0>(*proxy.begin()), 1.1);
  EXPECT_EQ(std::get<1>(*proxy.begin()), 1);
  std::get<0>(*proxy.begin()) = 0.0;
  EXPECT_EQ(std::get<0>(*proxy.begin()), 0.0);
}

TEST(EventListProxy, push_back) {
  std::vector<double> a{1.1, 2.2, 3.3};
  std::vector<int32_t> b{1, 2, 3};
  EventListProxy proxy(a, b);
  proxy.push_back(4.4, 4);
  EXPECT_EQ(std::get<0>(*(proxy.begin() + 3)), 4.4);
}
