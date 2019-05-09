// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#ifndef DATASET_TEST_COMMON_H
#define DATASET_TEST_COMMON_H

#include "test_macros.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <random>

#include "dataset.h"
#include "dimensions.h"

using namespace scipp;
using namespace scipp::core;

class Random {
  std::mt19937 mt{std::random_device()()};
  std::uniform_real_distribution<double> dist{-2.0, 2.0};

public:
  std::vector<double> operator()(const scipp::index size) {
    std::vector<double> data(size);
    std::generate(data.begin(), data.end(), [this]() { return dist(mt); });
    return data;
  }
};

Variable makeRandom(const Dimensions &dims);

/// Factory for creating datasets for testing. For a given instance, `make()`
/// will return datasets with identical coords and labels, such that they are
/// compatible in binary operations.
class DatasetFactory3D {
public:
  DatasetFactory3D(const scipp::index lx = 4, const scipp::index ly = 5,
                   const scipp::index lz = 6);

  Dataset make();

private:
  const scipp::index lx;
  const scipp::index ly;
  const scipp::index lz;
  Random rand;
  Dataset base;
};

#endif // DATASET_TEST_COMMON_H
