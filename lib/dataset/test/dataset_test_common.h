// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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

/// Factory for creating datasets for testing. For a given instance, `make()`
/// will return datasets with identical coords, such that they are compatible in
/// binary operations.
class DatasetFactory {
public:
  DatasetFactory();
  DatasetFactory(Dim dim, scipp::index length);
  DatasetFactory(std::initializer_list<std::pair<Dim, scipp::index>> dims);
  explicit DatasetFactory(Dimensions dims);

  void seed(uint32_t seed);
  Dataset make(std::string_view data_name = "data");
  Dataset make_with_random_masks(std::string_view data_name = "data");

  [[nodiscard]] const Dimensions &dims() const noexcept { return m_dims; }

private:
  void assign_coords(Dataset &dset);

  Dimensions m_dims;
  Random m_rand;
  RandomBool m_rand_bool;
};

template <class T, class T2>
auto make_1_coord(const Dim dim, const Dimensions &dims,
                  const sc_units::Unit unit,
                  const std::initializer_list<T2> &data) {
  return Dataset({{"a", makeVariable<T>(Dimensions(dims), sc_units::Unit(unit),
                                        Values(data))}},
                 {{dim, makeVariable<T>(Dimensions(dims), sc_units::Unit(unit),
                                        Values(data))}});
}

template <class T, class T2>
auto make_1_labels(const std::string &name, const Dimensions &dims,
                   const sc_units::Unit unit,
                   const std::initializer_list<T2> &data) {
  return Dataset(
      {{"a",
        makeVariable<T>(Dimensions(dims), sc_units::Unit(unit), Values(data))}},
      {{Dim(name), makeVariable<T>(Dimensions(dims), sc_units::Unit(unit),
                                   Values(data))}});
}

template <class T, class T2>
auto make_1_values(const std::string &name, const Dimensions &dims,
                   const sc_units::Unit unit,
                   const std::initializer_list<T2> &data) {
  return Dataset({{name, makeVariable<T>(Dimensions(dims), sc_units::Unit(unit),
                                         Values(data))}});
}

template <class T, class T2>
auto make_1_values_and_variances(const std::string &name,
                                 const Dimensions &dims,
                                 const sc_units::Unit unit,
                                 const std::initializer_list<T2> &values,
                                 const std::initializer_list<T2> &variances) {
  return Dataset(
      {{name, makeVariable<T>(Dimensions(dims), sc_units::Unit(unit),
                              Values(values), Variances(variances))}});
}

Dataset make_1d_masked();

namespace scipp::testdata {
Dataset make_dataset_x();
DataArray make_table(const scipp::index size);
} // namespace scipp::testdata
