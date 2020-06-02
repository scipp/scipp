// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <boost/container/small_vector.hpp>
#include <vector>

#include <scipp/dataset/dataset.h>

namespace scipp::dataset {

/// Implementation detail of GroupBy.
///
/// Stores the actual grouping details, independent of the container type.
class SCIPP_DATASET_EXPORT GroupByGrouping {
public:
  using group = boost::container::small_vector<Slice, 4>;
  GroupByGrouping(Variable &&key, std::vector<group> &&groups)
      : m_key(std::move(key)), m_groups(std::move(groups)) {}

  scipp::index size() const noexcept { return scipp::size(m_groups); }
  Dim dim() const noexcept { return m_key.dims().inner(); }
  const Variable &key() const noexcept { return m_key; }
  const std::vector<group> &groups() const noexcept { return m_groups; }

private:
  Variable m_key;
  std::vector<group> m_groups;
};

/// Helper class for implementing "split-apply-combine" functionality.
template <class T> class SCIPP_DATASET_EXPORT GroupBy {
public:
  GroupBy(const typename T::const_view_type &data, GroupByGrouping &&grouping)
      : m_data(data), m_grouping(std::move(grouping)) {}

  scipp::index size() const noexcept { return m_grouping.size(); }
  Dim dim() const noexcept { return m_grouping.dim(); }
  const Variable &key() const noexcept { return m_grouping.key(); }
  const std::vector<GroupByGrouping::group> &groups() const noexcept {
    return m_grouping.groups();
  }
  T copy(const scipp::index group,
         const AttrPolicy attrPolicy = AttrPolicy::Keep) const;

  T flatten(const Dim reductionDim) const;
  T mean(const Dim reductionDim) const;
  T sum(const Dim reductionDim) const;
  T all(const Dim reductionDim) const;
  T any(const Dim reductionDim) const;
  T max(const Dim reductionDim) const;
  T min(const Dim reductionDim) const;

private:
  T makeReductionOutput(const Dim reductionDim) const;
  template <class Op, class CoordOp = void *>
  T reduce(Op op, const Dim reductionDim, CoordOp coord_op = nullptr) const;

  typename T::const_view_type m_data;
  GroupByGrouping m_grouping;
};

SCIPP_DATASET_EXPORT GroupBy<DataArray>
groupby(const DataArrayConstView &dataset, const Dim dim);
SCIPP_DATASET_EXPORT GroupBy<DataArray>
groupby(const DataArrayConstView &dataset, const Dim dim,
        const VariableConstView &bins);

SCIPP_DATASET_EXPORT GroupBy<Dataset> groupby(const DatasetConstView &dataset,
                                              const Dim dim);
SCIPP_DATASET_EXPORT GroupBy<Dataset> groupby(const DatasetConstView &dataset,
                                              const Dim dim,
                                              const VariableConstView &bins);

SCIPP_DATASET_EXPORT GroupBy<DataArray>
groupby(const DataArrayConstView &dataset, const VariableConstView &variable,
        const VariableConstView &bins);

SCIPP_DATASET_EXPORT GroupBy<Dataset> groupby(const DatasetConstView &dataset,
                                              const VariableConstView &variable,
                                              const VariableConstView &bins);

} // namespace scipp::dataset
