// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#pragma once

#include "random.h"
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"

using namespace scipp;
using namespace scipp::dataset;

std::vector<bool> make_bools(const scipp::index size,
                             std::initializer_list<bool> pattern);
std::vector<bool> make_bools(const scipp::index size, bool pattern);

Variable makeRandom(const Dimensions &dims);

/// Factory for creating datasets for testing. For a given instance, `make()`
/// will return datasets with identical coords, such that they are compatible in
/// binary operations.
class DatasetFactory3D {
public:
  DatasetFactory3D(const scipp::index lx = 4, const scipp::index ly = 5,
                   const scipp::index lz = 6, const Dim dim = Dim::X);

  void seed(const uint32_t value);
  Dataset make(const bool randomMasks = false);

  const scipp::index lx;
  const scipp::index ly;
  const scipp::index lz;

private:
  void init();

  Dim m_dim;
  Random rand;
  RandomBool randBool;
  Dataset base;
};

Dataset make_empty();

template <class T, class T2>
auto make_1_coord(const Dim dim, const Dimensions &dims, const units::Unit unit,
                  const std::initializer_list<T2> &data) {
  auto d = make_empty();
  d.setCoord(
      dim, makeVariable<T>(Dimensions(dims), units::Unit(unit), Values(data)));
  return d;
}

template <class T, class T2>
auto make_1_labels(const std::string &name, const Dimensions &dims,
                   const units::Unit unit,
                   const std::initializer_list<T2> &data) {
  auto d = make_empty();
  d.setCoord(Dim(name), makeVariable<T>(Dimensions(dims), units::Unit(unit),
                                        Values(data)));
  return d;
}

template <class T, class T2>
auto make_1_attr(const std::string &name, const Dimensions &dims,
                 const units::Unit unit,
                 const std::initializer_list<T2> &data) {
  auto d = make_empty();
  d.setAttr(name,
            makeVariable<T>(Dimensions(dims), units::Unit(unit), Values(data)));
  return d;
}

template <class T, class T2>
auto make_1_values(const std::string &name, const Dimensions &dims,
                   const units::Unit unit,
                   const std::initializer_list<T2> &data) {
  auto d = make_empty();
  d.setData(name,
            makeVariable<T>(Dimensions(dims), units::Unit(unit), Values(data)));
  return d;
}

template <class T, class T2>
auto make_1_values_and_variances(const std::string &name,
                                 const Dimensions &dims, const units::Unit unit,
                                 const std::initializer_list<T2> &values,
                                 const std::initializer_list<T2> &variances) {
  auto d = make_empty();
  d.setData(name, makeVariable<T>(Dimensions(dims), units::Unit(unit),
                                  Values(values), Variances(variances)));
  return d;
}

Dataset make_simple_events(std::initializer_list<double> values,
                           std::string key = "events");

Dataset make_events_with_coords_and_labels(
    std::initializer_list<double> values,
    std::initializer_list<double> coords_and_labels,
    std::string key = "events");

Dataset make_events_2d(std::initializer_list<double> values,
                       std::string key = "events");

Dataset make_1d_masked();

namespace scipp::testdata {
Dataset make_dataset_x();
Dataset make_dataset_realigned_x_to_y();
} // namespace scipp::testdata
