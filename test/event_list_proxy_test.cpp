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
  EXPECT_EQ(std::get<1>(*(proxy.begin() + 3)), 4);
  proxy.push_back(*proxy.begin());
  EXPECT_EQ(std::get<0>(*(proxy.begin() + 4)), 1.1);
  EXPECT_EQ(std::get<1>(*(proxy.begin() + 4)), 1);
}

TEST(EventListProxy, push_back_3) {
  std::vector<double> a{1.1, 2.2, 3.3};
  std::vector<int32_t> b{1, 2, 3};
  std::vector<int32_t> c{3, 2, 1};
  EventListProxy proxy(a, b, c);
  proxy.push_back(4.4, 4, 1);
  EXPECT_EQ(std::get<0>(*(proxy.begin() + 3)), 4.4);
  EXPECT_EQ(std::get<1>(*(proxy.begin() + 3)), 4);
  EXPECT_EQ(std::get<2>(*(proxy.begin() + 3)), 1);
  proxy.push_back(*proxy.begin());
  EXPECT_EQ(std::get<0>(*(proxy.begin() + 4)), 1.1);
  EXPECT_EQ(std::get<1>(*(proxy.begin() + 4)), 1);
  EXPECT_EQ(std::get<2>(*(proxy.begin() + 4)), 3);
}

TEST(EventListProxy, push_back_duplicate_broken) {
  std::vector<double> a{1.1, 2.2, 3.3};
  std::vector<int32_t> b{1, 2, 3};

  // This is not allowed. We could add a check, but at this point it is not
  // clear if that is required, since creation should typically be under our
  // control, and we may want to avoid performance penalties.
  EventListProxy proxy(a, b, b);

  proxy.push_back(4.4, 4, 5);
  EXPECT_EQ(std::get<0>(*(proxy.begin() + 3)), 4.4);
  EXPECT_EQ(std::get<1>(*(proxy.begin() + 3)), 4);
  // b is now longer than a, we view the wrong element.
  EXPECT_EQ(std::get<2>(*(proxy.begin() + 3)), 4);
}
