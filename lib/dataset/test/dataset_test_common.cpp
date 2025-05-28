// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <algorithm>

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/astype.h"

#include "dataset_test_common.h"

std::vector<bool> make_bools(const scipp::index size,
                             std::initializer_list<bool> pattern) {
  std::vector<bool> result(size);
  auto it = pattern.begin();
  for (auto &&itm : result) {
    if (it == pattern.end())
      it = pattern.begin();
    itm = *(it++);
  }
  return result;
}
std::vector<bool> make_bools(const scipp::index size, bool pattern) {
  return make_bools(size, std::initializer_list<bool>{pattern});
}

DatasetFactory::DatasetFactory()
    : DatasetFactory(Dimensions{{Dim::X, 4}, {Dim::Y, 3}, {Dim::Z, 5}}) {}

DatasetFactory::DatasetFactory(const scipp::sc_units::Dim dim,
                               const scipp::index length)
    : DatasetFactory(Dimensions({{dim, length}})) {}

DatasetFactory::DatasetFactory(
    const std::initializer_list<std::pair<Dim, scipp::index>> dims)
    : DatasetFactory(Dimensions(dims)) {}

DatasetFactory::DatasetFactory(Dimensions dims) : m_dims{std::move(dims)} {
  seed(549634198);
}

void DatasetFactory::seed(const uint32_t seed) {
  m_rand.seed(seed);
  m_rand_bool.seed(seed);
}

Dataset DatasetFactory::make(const std::string_view data_name) {
  const std::string name{data_name};
  DataArray data(
      makeVariable<double>(m_dims, Values(m_rand(m_dims.volume()))), {},
      {{"mask",
        makeVariable<bool>(
            m_dims, Values(make_bools(m_dims.volume(), {true, false})))}});
  Dataset result({{name, std::move(data)}});
  assign_coords(result);
  return result;
}

Dataset
DatasetFactory::make_with_random_masks(const std::string_view data_name) {
  auto result = make(data_name);
  result[std::string(data_name)].masks().set(
      "mask", makeVariable<bool>(m_dims, Values(m_rand_bool(m_dims.volume()))));
  return result;
}

void DatasetFactory::assign_coords(Dataset &dset) {
  dset.setCoord(Dim{"scalar"}, makeVariable<double>(Values{1.2}));
  for (const auto dim : m_dims) {
    const auto length = m_dims[dim];
    dset.setCoord(dim, makeVariable<double>(Dimensions{dim, length},
                                            Values(m_rand(length))));
    dset.setCoord(
        Dim("labels_" + to_string(dim)),
        makeVariable<double>(Dimensions{dim, length}, Values(m_rand(length))));
  }
  if (m_dims.ndim() > 1) {
    const std::vector dims(m_dims.begin(), m_dims.end());
    dset.setCoord(Dim{to_string(dims[0]) + to_string(dims[1])},
                  makeVariable<double>(
                      Dims{dims[0], dims[1]},
                      Shape{m_dims[dims[0]], m_dims[dims[1]]},
                      Values(m_rand(m_dims[dims[0]] * m_dims[dims[1]]))));
  }
}

Dataset make_1d_masked() {
  Random random;
  Dataset ds({{"data_x", makeVariable<double>(Dimensions{Dim::X, 10},
                                              Values(random(10)))}});
  ds["data_x"].masks().set(
      "masks_x", makeVariable<bool>(Dimensions{Dim::X, 10},
                                    Values(make_bools(10, {false, true}))));
  return ds;
}

namespace scipp::testdata {

Dataset make_dataset_x() {
  return Dataset({{"a", makeVariable<double>(Dims{Dim::X}, sc_units::kg,
                                             Shape{3}, Values{4, 5, 6})},
                  {"b", makeVariable<int32_t>(Dims{Dim::X}, sc_units::s,
                                              Shape{3}, Values{7, 8, 9})}},
                 {{Dim("scalar"), 1.2 * sc_units::K},
                  {Dim::X, makeVariable<double>(Dims{Dim::X}, sc_units::m,
                                                Shape{3}, Values{1, 2, 4})},
                  {Dim::Y, makeVariable<double>(Dims{Dim::X}, sc_units::m,
                                                Shape{3}, Values{1, 2, 3})}});
}

DataArray make_table(const scipp::index size) {
  Random rand;
  rand.seed(0);
  const Dimensions dims(Dim::Row, size);
  const auto data = makeVariable<double>(dims, Values(rand(dims.volume())),
                                         Variances(rand(dims.volume())));
  const auto x = makeVariable<double>(dims, Values(rand(dims.volume())));
  const auto y = makeVariable<double>(dims, Values(rand(dims.volume())));
  const auto group = astype(
      makeVariable<double>(dims, Values(rand(dims.volume()))), dtype<int64_t>);
  const auto group2 = astype(
      makeVariable<double>(dims, Values(rand(dims.volume()))), dtype<int64_t>);
  return DataArray(data, {{Dim::X, x},
                          {Dim::Y, y},
                          {Dim("group"), group},
                          {Dim("group2"), group2}});
}

} // namespace scipp::testdata
