// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <boost/container/small_vector.hpp>
#include <vector>

#include "scipp/core/flags.h"
#include "scipp/variable/creation.h"
#include <scipp/dataset/dataset.h>

namespace scipp::dataset {

/// Implementation detail of GroupBy.
///
/// Stores the actual grouping details, independent of the container type.
class SCIPP_DATASET_EXPORT GroupByGrouping {
public:
  using group = boost::container::small_vector<Slice, 4>;
  GroupByGrouping(Variable key, std::vector<group> groups)
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
  GroupBy(const T &data, GroupByGrouping &&grouping)
      : m_data(data), m_grouping(std::move(grouping)) {}

  scipp::index size() const noexcept { return m_grouping.size(); }
  Dim dim() const noexcept { return m_grouping.dim(); }
  const Variable &key() const noexcept { return m_grouping.key(); }
  const std::vector<GroupByGrouping::group> &groups() const noexcept {
    return m_grouping.groups();
  }
  T copy(const scipp::index group,
         const AttrPolicy attrPolicy = AttrPolicy::Keep) const;

  T concat(const Dim reductionDim) const;
  T mean(const Dim reductionDim) const;
  T sum(const Dim reductionDim) const;
  T all(const Dim reductionDim) const;
  T any(const Dim reductionDim) const;
  T max(const Dim reductionDim) const;
  T min(const Dim reductionDim) const;
  T copy(const SortOrder order) const;

private:
  T makeReductionOutput(const Dim reductionDim, const FillValue fill) const;
  template <class Op>
  T reduce(Op op, const Dim reductionDim, const FillValue fill) const;

  T m_data;
  GroupByGrouping m_grouping;
};

SCIPP_DATASET_EXPORT GroupBy<DataArray> groupby(const DataArray &dataset,
                                                const Dim dim);
SCIPP_DATASET_EXPORT GroupBy<DataArray>
groupby(const DataArray &dataset, const Dim dim, const Variable &bins);

SCIPP_DATASET_EXPORT GroupBy<Dataset> groupby(const Dataset &dataset,
                                              const Dim dim);
SCIPP_DATASET_EXPORT GroupBy<Dataset>
groupby(const Dataset &dataset, const Dim dim, const Variable &bins);

SCIPP_DATASET_EXPORT GroupBy<DataArray> groupby(const DataArray &dataset,
                                                const Variable &variable,
                                                const Variable &bins);

SCIPP_DATASET_EXPORT GroupBy<Dataset>
groupby(const Dataset &dataset, const Variable &variable, const Variable &bins);

} // namespace scipp::dataset
