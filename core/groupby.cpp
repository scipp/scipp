// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/core/except.h"
#include "scipp/core/groupby.h"
#include "scipp/core/indexed_slice_view.h"
#include "scipp/core/tag_util.h"

#include "dataset_operations_common.h"

namespace scipp::core {

Dataset GroupBy::mean(const Dim dim) const {
  // 1. Prepare output dataset
  constexpr auto do_slice = [](const VariableConstProxy &var, auto &&... args) {
    return Variable(var.slice({args...}));
  };
  constexpr auto slice_array = [](auto &&... _) {
    return apply_to_data_and_drop_dim(_...);
  };
  // Delete anything (but data) that depends on the mean dimension `dim`.
  Dataset out = apply_to_items(m_data, slice_array, do_slice, dim, 0,
                               scipp::size(m_groups));
  const Dim outDim = m_key.dims().inner();
  out.rename(dim, outDim);

  // 2. Apply to each group, storing result in output slice
  scipp::index i = 0;
  for (const auto &group : m_groups) {
    const auto out_slice = out.slice({outDim, i++});
    for (scipp::index slice = 0; slice < scipp::size(group); ++slice) {
      const auto &data_slice = m_data.slice({dim, group[slice]});
      if (slice == 0)
        out_slice.assign(data_slice);
      else
        out_slice += data_slice;
    }
    out_slice /= static_cast<double>(scipp::size(group));
  }

  // 3. Set the group labels as new coord
  out.setCoord(outDim, m_key);

  return out;
}

template <class T> struct MakeGroups {
  static auto apply(const DatasetConstProxy &d, const std::string &labels,
                    const Dim targetDim) {
    const auto &key = d.labels()[labels];
    if (key.dims().ndim() != 1)
      throw except::DimensionError("Group-by key must be 1-dimensional");
    if (key.hasVariances())
      throw except::VariancesError("Group-by key cannot have variances");
    const auto &values = key.values<T>();

    std::map<T, std::vector<scipp::index>> indices;
    for (scipp::index i = 0; i < scipp::size(values); ++i)
      indices[values[i]].push_back(i);
    // TODO Better just return indices, without copying data into groups.
    // Alternatively return map to IndexedSliceView?

    const Dimensions dims{targetDim, scipp::size(indices)};
    Vector<T> keys;
    std::vector<std::vector<scipp::index>> groups;
    for (auto &item : indices) {
      keys.push_back(item.first);
      groups.emplace_back(std::move(item.second));
    }
    auto keys_ = makeVariable<T>(dims, std::move(keys));
    keys_.setUnit(key.unit());
    return GroupBy{d, std::move(keys_), std::move(groups)};
  }
};

GroupBy groupby(const DatasetConstProxy &dataset, const std::string &labels,
                const Dim targetDim) {
  return CallDType<double, float, int64_t, int32_t, bool, std::string>::apply<
      MakeGroups>(dataset.labels()[labels].dtype(), dataset, labels, targetDim);
}

} // namespace scipp::core
