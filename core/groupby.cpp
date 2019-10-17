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
#include "variable_operations_common.h"

namespace scipp::core {

Dataset GroupBy::sum(const Dim dim) const {
  // 1. Prepare output dataset:
  // - Delete anything (but data) that depends on the reduction dimension `dim`.
  // - Default-init data.
  constexpr auto init_var = [](const VariableConstProxy &var, const Dim dim_,
                               const scipp::index size_) {
    auto dims = var.dims();
    dims.resize(dim_, size_);
    return Variable(var, dims);
  };
  Dataset out = apply_to_items(
      m_data, [](auto &&... _) { return apply_to_data_and_drop_dim(_...); },
      init_var, dim, size());
  const Dim outDim = this->dim();
  out.rename(dim, outDim);
  out.setCoord(outDim, m_key);

  // 2. Apply to each group, storing result in output slice
  for (scipp::index group = 0; group < size(); ++group) {
    const auto out_slice = out.slice({outDim, group});
    for (const auto & [ name, item ] : m_data) {
      const auto out_data = out_slice[name].data();
      for (const auto &slice : m_groups[group]) {
        sum_impl(out_data, item.data().slice(slice));
      }
    }
  }
  return out;
}

Dataset GroupBy::mean(const Dim dim) const {
  // 1. Sum into output slices
  auto out = sum(dim);

  // 2. Compute number of slices N contributing to each out slice
  const Dim outDim = this->dim();
  auto scale = makeVariable<double>({outDim, size()});
  const auto scaleT = scale.values<double>();
  for (scipp::index group = 0; group < size(); ++group)
    for (const auto &slice : m_groups[group])
      scaleT[group] += slice.end() - slice.begin();
  scale = 1.0 / scale;

  // 3. sum/N -> mean
  for (const auto & [ name, item ] : out) {
    if (isInt(item.data().dtype()))
      out.setData(name, item.data() * scale);
    else
      item *= scale;
  }

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

    const auto dim = key.dims().inner();
    std::map<T, std::vector<Slice>> indices;
    for (scipp::index i = 0; i < scipp::size(values);) {
      // Use contiguous (thick) slices if possible to avoid overhead of slice
      // handling in follow-up "apply" steps.
      const auto begin = i;
      const auto value = values[i];
      while (values[i] == value)
        ++i;
      indices[value].emplace_back(dim, begin, i);
    }

    const Dimensions dims{targetDim, scipp::size(indices)};
    Vector<T> keys;
    std::vector<std::vector<Slice>> groups;
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
