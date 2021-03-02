// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#pragma once

#include "test_macros.h"
#include "test_random.h"
#include <gtest/gtest.h>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"

std::vector<bool> make_bools(const scipp::index size,
                             std::initializer_list<bool> pattern);
std::vector<bool> make_bools(const scipp::index size, bool pattern);

/// Factory for creating datasets for testing. For a given instance, `make()`
/// will return datasets with identical coords, such that they are compatible in
/// binary operations.
class DatasetFactory3D {
public:
  DatasetFactory3D(const scipp::index lx = 4, const scipp::index ly = 5,
                   const scipp::index lz = 6,
                   const scipp::Dim dim = scipp::Dim::X);

  void seed(const uint32_t value);
  scipp::dataset::Dataset make(const bool randomMasks = false);

  const scipp::index lx;
  const scipp::index ly;
  const scipp::index lz;

private:
  void init();

  scipp::Dim m_dim;
  scipp::testing::Random rand;
  scipp::testing::RandomBool randBool;
  scipp::dataset::Dataset base;
};

scipp::dataset::Dataset make_empty();

template <class T, class T2>
auto make_1_coord(const scipp::Dim dim, const scipp::Dimensions &dims,
                  const scipp::units::Unit unit,
                  const std::initializer_list<T2> &data) {
  auto d = make_empty();
  d.setCoord(dim, scipp::makeVariable<T>(scipp::Dimensions(dims),
                                         scipp::units::Unit(unit),
                                         scipp::Values(data)));
  return d;
}

template <class T, class T2>
auto make_1_labels(const std::string &name, const scipp::Dimensions &dims,
                   const scipp::units::Unit unit,
                   const std::initializer_list<T2> &data) {
  auto d = make_empty();
  d.setCoord(scipp::Dim(name), scipp::makeVariable<T>(scipp::Dimensions(dims),
                                                      scipp::units::Unit(unit),
                                                      scipp::Values(data)));
  return d;
}

template <class T, class T2>
auto make_1_values(const std::string &name, const scipp::Dimensions &dims,
                   const scipp::units::Unit unit,
                   const std::initializer_list<T2> &data) {
  auto d = make_empty();
  d.setData(name, scipp::makeVariable<T>(scipp::Dimensions(dims),
                                         scipp::units::Unit(unit),
                                         scipp::Values(data)));
  return d;
}

template <class T, class T2>
auto make_1_values_and_variances(const std::string &name,
                                 const scipp::Dimensions &dims,
                                 const scipp::units::Unit unit,
                                 const std::initializer_list<T2> &values,
                                 const std::initializer_list<T2> &variances) {
  auto d = make_empty();
  d.setData(name, scipp::makeVariable<T>(
                      scipp::Dimensions(dims), scipp::units::Unit(unit),
                      scipp::Values(values), scipp::Variances(variances)));
  return d;
}

scipp::dataset::Dataset make_1d_masked();

namespace scipp::testdata {
Dataset make_dataset_x();

DataArray make_table(
    scipp::index size, bool with_variances = true,
    std::tuple<core::DType, core::DType, core::DType, core::DType, core::DType>
        dtypes = std::tuple{core::dtype<double>, core::dtype<double>,
                            core::dtype<double>, core::dtype<int64_t>,
                            core::dtype<int64_t>},
    std::optional<uint32_t> seed = std::nullopt);
} // namespace scipp::testdata
