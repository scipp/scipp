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
  // TODO Eliminate the `seed` argument and the local `mt` once all tests have
  // been refactored to work with random data.
  std::vector<double> operator()(const int seed, const scipp::index size) {
    std::mt19937 mt{seed};
    std::vector<double> data(size);
    std::generate(data.begin(), data.end(), [this, &mt]() { return dist(mt); });
    return data;
  }
  std::vector<double> operator()(const scipp::index size) {
    std::vector<double> data(size);
    std::generate(data.begin(), data.end(), [this]() { return dist(mt); });
    return data;
  }
};

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

class Dataset3DTest : public ::testing::Test {
private:
  Random m_rand;

protected:
  Dataset3DTest() {
    dataset.setCoord(Dim::Time, scalar());
    dataset.setCoord(Dim::X, x());
    dataset.setCoord(Dim::Y, y());
    dataset.setCoord(Dim::Z, xyz());

    dataset.setLabels("labels_x", x());
    dataset.setLabels("labels_xy", xy());
    dataset.setLabels("labels_z", z());

    dataset.setAttr("attr_scalar", scalar());
    dataset.setAttr("attr_x", x());

    dataset.setValues("data_x", x());
    dataset.setVariances("data_x", x());

    dataset.setValues("data_xy", xy());
    dataset.setVariances("data_xy", xy());

    dataset.setValues("data_zyx", zyx());
    dataset.setVariances("data_zyx", zyx());

    dataset.setValues("data_xyz", xyz());

    dataset.setValues("data_scalar", scalar());
  }

  Variable scalar() { return makeVariable<double>({}, m_rand(0, 1)); }
  Variable x(const scipp::index lx = 4) {
    return makeVariable<double>({Dim::X, lx}, m_rand(1, lx));
  }
  Variable y(const scipp::index ly = 5) {
    return makeVariable<double>({Dim::Y, ly}, m_rand(2, ly));
  }
  Variable z() { return makeVariable<double>({Dim::Z, 6}, m_rand(3, 6)); }
  Variable xy(const scipp::index lx = 4, const scipp::index ly = 5) {
    return makeVariable<double>({{Dim::X, lx}, {Dim::Y, ly}},
                                m_rand(4, lx * ly));
  }
  Variable xyz(const scipp::index lz = 6) {
    return makeVariable<double>({{Dim::X, 4}, {Dim::Y, 5}, {Dim::Z, lz}},
                                m_rand(5, 4 * 5 * lz));
  }
  Variable zyx() {
    return makeVariable<double>({{Dim::Z, 6}, {Dim::Y, 5}, {Dim::X, 4}},
                                m_rand(6, 4 * 5 * 6));
  }

  Dataset datasetWithEdges(const std::initializer_list<Dim> &edgeDims) {
    auto d = dataset;
    for (const auto dim : edgeDims) {
      auto dims = dataset.coords()[dim].dims();
      dims.resize(dim, dims[dim] + 1);
      d.setCoord(dim, makeVariable<double>(dims, m_rand(7, dims.volume())));
    }
    return d;
  }

  Dataset dataset;
};

#endif // DATASET_TEST_COMMON_H
