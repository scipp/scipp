/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include <algorithm>
#include <random>

#include "range/v3/algorithm.hpp"

#include "test_macros.h"

#include "dataset.h"
#include "zip_view.h"

TEST(ZipView, construct_fail) {
  Dataset d;

  d.insert(Coord::X{}, {Dim::X, 3});
  d.insert(Data::Value{}, "", {Dim::X, 3});
  EXPECT_THROW_MSG(
      ZipView<Coord::X> view(d), std::runtime_error,
      "ZipView must be constructed based on *all* variables in a dataset.");
  d.erase(Data::Value{});

  d.insert(Data::Value{}, "", {});
  EXPECT_THROW_MSG((ZipView<Coord::X, Data::Value>(d)), std::runtime_error,
                   "ZipView supports only datasets where all variables are "
                   "1-dimensional.");
  d.erase(Data::Value{});

  d.insert(Coord::Y{}, {Dim::Y, 3});
  EXPECT_THROW_MSG((ZipView<Coord::X, Coord::Y>(d)), std::runtime_error,
                   "ZipView supports only 1-dimensional datasets.");
}

TEST(ZipView, construct) {
  Dataset d;
  d.insert(Coord::X{}, {Dim::X, 3});
  EXPECT_NO_THROW(ZipView<Coord::X> view(d));
}

TEST(ZipView, push_back_1_variable) {
  Dataset d;
  d.insert(Coord::X{}, {Dim::X, 3});
  ZipView<Coord::X> view(d);
  view.push_back({1.1});
  ASSERT_EQ(d.get<Coord::X>().size(), 4);
  ASSERT_EQ(d(Coord::X{}).dimensions().size(0), 4);
  view.push_back(2.2);
  ASSERT_EQ(d.get<Coord::X>().size(), 5);
  ASSERT_EQ(d(Coord::X{}).dimensions().size(0), 5);
  const auto data = d.get<Coord::X>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], 0.0);
  EXPECT_EQ(data[3], 1.1);
  EXPECT_EQ(data[4], 2.2);
}

TEST(ZipView, push_back_2_variables) {
  Dataset d;
  d.insert(Coord::X{}, {Dim::X, 2});
  d.insert(Data::Value{}, "", {Dim::X, 2});
  ZipView<Coord::X, Data::Value> view(d);
  view.push_back({1.1, 1.2});
  ASSERT_EQ(d.get<Coord::X>().size(), 3);
  ASSERT_EQ(d(Coord::X{}).dimensions().size(0), 3);
  view.push_back({2.2, 2.3});
  ASSERT_EQ(d.get<Coord::X>().size(), 4);
  ASSERT_EQ(d(Coord::X{}).dimensions().size(0), 4);

  const auto coord = d.get<Coord::X>();
  EXPECT_EQ(coord[0], 0.0);
  EXPECT_EQ(coord[1], 0.0);
  EXPECT_EQ(coord[2], 1.1);
  EXPECT_EQ(coord[3], 2.2);
  const auto data = d.get<Data::Value>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], 1.2);
  EXPECT_EQ(data[3], 2.3);
}

TEST(ZipView, std_algorithm_generate_n_with_back_inserter) {
  Dataset d;
  d.insert(Coord::X{}, {Dim::X, 0});
  d.insert(Data::Value{}, "", {Dim::X, 0});

  ZipView<Coord::X, Data::Value> view(d);

  std::mt19937 rng;
  std::generate_n(std::back_inserter(view), 5, [&] {
    const double v = rng();
    const double x = rng();
    return std::make_tuple(x, v);
  });

  ASSERT_EQ(d.get<Coord::X>().size(), 5);
  ASSERT_EQ(d(Coord::X{}).dimensions().size(0), 5);
  ASSERT_EQ(d.get<Data::Value>().size(), 5);
  ASSERT_EQ(d(Data::Value{}).dimensions().size(0), 5);

  rng = std::mt19937{};
  for (const auto x : d.get<Coord::X>()) {
    rng();
    EXPECT_EQ(x, rng());
  }
  rng = std::mt19937{};
  for (const auto v : d.get<Data::Value>()) {
    EXPECT_EQ(v, rng());
    rng();
  }
}

TEST(ZipView, iterator_1_variable) {
  Dataset d;
  d.insert(Coord::X{}, {Dim::X, 3}, {1.0, 2.0, 3.0});
  ZipView<Coord::X> view(d);
  EXPECT_EQ(std::distance(view.begin(), view.end()), 3);
  auto it = view.begin();
  EXPECT_EQ(std::get<0>(*it++), 1.0);
  EXPECT_EQ(std::get<0>(*it++), 2.0);
  EXPECT_EQ(std::get<0>(*it++), 3.0);
  EXPECT_EQ(it, view.end());
}

TEST(ZipView, iterator_modify) {
  Dataset d;
  d.insert(Coord::X{}, {Dim::X, 3}, {1.0, 2.0, 3.0});
  d.insert(Data::Value{}, "", {Dim::X, 3}, {1.1, 2.1, 3.1});
  ZipView<Coord::X, Data::Value> view(d);

  // Note this peculiarity: `item` is returned by value but it is a proxy
  // object, i.e., it containts references that can be used to modify the
  // dataset.
  for (auto item : view)
    std::get<1>(item) *= 2.0;

  EXPECT_TRUE(equals(d.get<Coord::X>(), {1.0, 2.0, 3.0}));
  EXPECT_TRUE(equals(d.get<Data::Value>(), {2.2, 4.2, 6.2}));
}

TEST(ZipView, iterator_copy) {
  Dataset source;
  source.insert(Coord::X{}, {Dim::X, 3}, {1.0, 2.0, 3.0});
  source.insert(Data::Value{}, "", {Dim::X, 3}, {1.1, 2.1, 3.1});
  ZipView<Coord::X, Data::Value> source_view(source);

  Dataset d;
  d.insert(Coord::X{}, {Dim::X, 0});
  d.insert(Data::Value{}, "", {Dim::X, 0});
  ZipView<Coord::X, Data::Value> view(d);

  std::copy(source_view.begin(), source_view.end(), std::back_inserter(view));
  std::copy(source_view.begin(), source_view.end(), std::back_inserter(view));

  EXPECT_TRUE(equals(d.get<Coord::X>(), {1.0, 2.0, 3.0, 1.0, 2.0, 3.0}));
  EXPECT_TRUE(equals(d.get<Data::Value>(), {1.1, 2.1, 3.1, 1.1, 2.1, 3.1}));

  std::copy(source_view.begin(), source_view.end(), view.begin() + 1);

  EXPECT_TRUE(equals(d.get<Coord::X>(), {1.0, 1.0, 2.0, 3.0, 2.0, 3.0}));
  EXPECT_TRUE(equals(d.get<Data::Value>(), {1.1, 1.1, 2.1, 3.1, 2.1, 3.1}));
}

TEST(ZipView, iterator_copy_if) {
  Dataset source;
  source.insert(Coord::X{}, {Dim::X, 3}, {1.0, 2.0, 3.0});
  source.insert(Data::Value{}, "", {Dim::X, 3}, {1.1, 2.1, 3.1});
  ZipView<Coord::X, Data::Value> source_view(source);

  Dataset d;
  d.insert(Coord::X{}, {Dim::X, 0});
  d.insert(Data::Value{}, "", {Dim::X, 0});
  ZipView<Coord::X, Data::Value> view(d);

  std::copy_if(source_view.begin(), source_view.end(), std::back_inserter(view),
               [](const auto &item) { return std::get<1>(item) > 2.0; });

  EXPECT_TRUE(equals(d.get<Coord::X>(), {2.0, 3.0}));
  EXPECT_TRUE(equals(d.get<Data::Value>(), {2.1, 3.1}));

  std::copy_if(source_view.begin(), source_view.end(), std::back_inserter(view),
               [](const auto &item) { return std::get<1>(item) > 2.0; });

  EXPECT_TRUE(equals(d.get<Coord::X>(), {2.0, 3.0, 2.0, 3.0}));
  EXPECT_TRUE(equals(d.get<Data::Value>(), {2.1, 3.1, 2.1, 3.1}));
}

TEST(ZipView, iterator_sort) {
  Dataset d;
  d.insert(Coord::X{}, {Dim::X, 4}, {3.0, 2.0, 1.0, 0.0});
  ZipView<Coord::X> view(d);

  // Note: Unlike other std algorithms, std::sort does not work with these
  // iterators, must use ranges::sort.
  using ranges::sort;
  sort(view.begin(), view.end(), [](const auto &a, const auto &b) {
    return std::get<0>(a) < std::get<0>(b);
  });

  EXPECT_TRUE(equals(d.get<Coord::X>(), {0.0, 1.0, 2.0, 3.0}));
}
