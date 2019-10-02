// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/core/except.h"
#include "scipp/core/groupby.h"
#include "scipp/core/indexed_slice_view.h"
#include "scipp/core/tag_util.h"

namespace scipp::core {

template <class T> struct MakeGroups {
  static auto apply(const DatasetConstProxy &d, const std::string &labels) {
    const auto &key = d.labels()[labels];
    if (key.dims().ndim() != 1)
      throw except::DimensionError("Group-by key must be 1-dimensional");
    if (key.hasVariances())
      throw except::VariancesError("Group-by key cannot have variances");
    const auto &values = key.values<T>();

    std::map<T, std::vector<scipp::index>> indices;
    for (scipp::index i = 0; i < scipp::size(values); ++i)
      indices[values[i]].push_back(i);

    const Dim dim = key.dims().inner();
    const Dimensions dims{dim, scipp::size(indices)};
    Vector<T> keys;
    Vector<Dataset> groups;
    for (const auto &item : indices) {
      keys.push_back(item.first);
      groups.emplace_back(concatenate(IndexedSliceView{d, dim, item.second}));
    }
    auto keys_ = makeVariable<T>(dims, std::move(keys));
    keys_.setUnit(key.unit());
    return DataArray(makeVariable<Dataset>(dims, std::move(groups)), {},
                     {{labels, std::move(keys_)}});
  }
};

static auto makeGroups(const DatasetConstProxy &dataset,
                       const std::string &labels) {
  return CallDType<double, float, int64_t, int32_t, bool, std::string>::apply<
      MakeGroups>(dataset.labels()[labels].dtype(), dataset, labels);
}

DataArray groupby(const DatasetConstProxy &dataset, const std::string &labels) {
  return makeGroups(dataset, labels);
}

} // namespace scipp::core
