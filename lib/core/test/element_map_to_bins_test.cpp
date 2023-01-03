// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

#include "scipp/core/element/map_to_bins.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <iterator>
#include <random>
#include <vector>

using namespace scipp;
using namespace scipp::core::element;

auto random_shuffled(const unsigned int seed, const scipp::index nevent,
                     const scipp::index nbin) {
  std::mt19937 mersenne_engine{seed};
  std::uniform_int_distribution<scipp::index> dist{0, nbin - 1};
  auto gen = [&dist, &mersenne_engine]() { return dist(mersenne_engine); };
  std::vector<scipp::index> vec(nevent);
  std::generate(begin(vec), end(vec), gen);
  return vec;
}

class ElementMapToBinsTest : public ::testing::Test {
protected:
  ElementMapToBinsTest(scipp::index nevent_ = 1033, scipp::index nbin_ = 17)
      : nevent(nevent_), nbin(nbin_), binned(nevent),
        bin_indices(random_shuffled(seed, nevent, nbin)),
        data(bin_indices.begin(), bin_indices.end()) {
    scipp::index current = 0;
    for (scipp::index i = 0; i < nbin; ++i) {
      bins.push_back(current);
      current += std::count(bin_indices.begin(), bin_indices.end(), i);
    }
  }
  unsigned int seed = std::random_device()();
  const scipp::index nevent;
  const scipp::index nbin;
  std::vector<double> binned;
  std::vector<scipp::index> bins;
  std::vector<scipp::index> bin_indices;
  std::vector<double> data;
};

TEST_F(ElementMapToBinsTest, data_matching_index_equivalent_to_sort) {
  map_to_bins_direct(binned, bins, data, bin_indices);
  std::sort(data.begin(), data.end());
  EXPECT_EQ(binned, data) << seed;
}

class ElementMapToBinsChunkedTest
    : public ElementMapToBinsTest,
      public ::testing::WithParamInterface<
          std::tuple<scipp::index, scipp::index>> {
protected:
  ElementMapToBinsChunkedTest()
      : ElementMapToBinsTest(std::get<0>(GetParam()), std::get<1>(GetParam())) {
  }

  template <int N> void check_direct_equivalent_to_chunkwise() {
    auto binned1 = binned;
    auto binned2 = binned;
    auto bins1 = bins;
    auto bins2 = bins;
    map_to_bins_direct(binned1, bins1, data, bin_indices);
    map_to_bins_chunkwise<N>(binned2, bins2, data, bin_indices);
    EXPECT_EQ(binned1, binned2) << seed;
  }
};

INSTANTIATE_TEST_SUITE_P(NEventNBin, ElementMapToBinsChunkedTest,
                         testing::Combine(testing::Values(9000, 1033),
                                          testing::Values(70000, 7000,
                                                          128 * 128, 17)));

TEST_P(ElementMapToBinsChunkedTest, direct_equivalent_to_chunkwise) {
  check_direct_equivalent_to_chunkwise<1>();
  check_direct_equivalent_to_chunkwise<2>();
  check_direct_equivalent_to_chunkwise<4>();
  check_direct_equivalent_to_chunkwise<16>();
  check_direct_equivalent_to_chunkwise<64>();
  check_direct_equivalent_to_chunkwise<256>();
  check_direct_equivalent_to_chunkwise<512>();
  check_direct_equivalent_to_chunkwise<1024>();
  check_direct_equivalent_to_chunkwise<2048>();
}
