// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

#include "scipp/core/element/map_to_bins.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <iterator>
#include <random>
#include <vector>

using namespace scipp;
using namespace scipp::core::element;

auto random_shuffled(const scipp::index nevent, const scipp::index nbin) {
  std::random_device rnd_device;
  std::mt19937 mersenne_engine{rnd_device()};
  std::uniform_int_distribution<scipp::index> dist{0, nbin - 1};
  auto gen = [&dist, &mersenne_engine]() { return dist(mersenne_engine); };
  std::vector<scipp::index> vec(nevent);
  std::generate(begin(vec), end(vec), gen);
  std::shuffle(begin(vec), end(vec), mersenne_engine);
  return vec;
}

class ElementMapToBinsTest : public ::testing::Test {
protected:
  ElementMapToBinsTest()
      : binned(nevent), bin_indices(random_shuffled(nevent, nbin)),
        data(bin_indices.begin(), bin_indices.end()) {
    scipp::index current = 0;
    for (scipp::index i = 0; i < nbin; ++i) {
      bins.push_back(current);
      current += std::count(bin_indices.begin(), bin_indices.end(), i);
    }
  }
  const scipp::index nevent = 1033;
  const scipp::index nbin = 17;
  std::vector<double> binned;
  std::vector<scipp::index> bins;
  std::vector<scipp::index> bin_indices;
  std::vector<double> data;

  template <int N> void check_direct_equivalent_to_chunkwise() {
    auto binned1 = binned;
    auto binned2 = binned;
    auto bins1 = bins;
    auto bins2 = bins;
    map_to_bins_direct(binned1, bins1, data, bin_indices);
    map_to_bins_chunkwise<N>(binned2, bins2, data, bin_indices);
    EXPECT_EQ(binned1, binned2);
  }
};

TEST_F(ElementMapToBinsTest, data_matching_index_equivalent_to_sort) {
  map_to_bins_direct(binned, bins, data, bin_indices);
  std::sort(data.begin(), data.end());
  EXPECT_EQ(binned, data);
}

TEST_F(ElementMapToBinsTest, direct_equivalent_to_chunkwise) {
  check_direct_equivalent_to_chunkwise<1>();
  check_direct_equivalent_to_chunkwise<2>();
  check_direct_equivalent_to_chunkwise<4>();
  check_direct_equivalent_to_chunkwise<16>();
  check_direct_equivalent_to_chunkwise<64>();
  check_direct_equivalent_to_chunkwise<256>();
}
