// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_GROUPBY_H
#define SCIPP_CORE_GROUPBY_H

#include <vector>

#include <scipp/core/dataset.h>

namespace scipp::core {

/// Implementation detail of GroupBy.
///
/// Stores the actual grouping details, independent of the container type.
class SCIPP_CORE_EXPORT GroupByGrouping {
public:
  GroupByGrouping(Variable &&key, std::vector<std::vector<Slice>> &&groups)
      : m_key(std::move(key)), m_groups(std::move(groups)) {}

  scipp::index size() const noexcept { return scipp::size(m_groups); }
  Dim dim() const noexcept { return m_key.dims().inner(); }
  const Variable &key() const noexcept { return m_key; }
  const std::vector<std::vector<Slice>> &groups() const noexcept {
    return m_groups;
  }

private:
  Variable m_key;
  std::vector<std::vector<Slice>> m_groups;
};

/// Helper class for implementing "split-apply-combine" functionality.
template <class T> class SCIPP_CORE_EXPORT GroupBy {
public:
  GroupBy(const typename T::const_view_type &data, GroupByGrouping &&grouping)
      : m_data(data), m_grouping(std::move(grouping)) {}

  scipp::index size() const noexcept { return m_grouping.size(); }
  Dim dim() const noexcept { return m_grouping.dim(); }
  const Variable &key() const noexcept { return m_grouping.key(); }
  const std::vector<std::vector<Slice>> &groups() const noexcept {
    return m_grouping.groups();
  }

  T flatten(const Dim reductionDim) const;
  T mean(const Dim reductionDim) const;
  T sum(const Dim reductionDim) const;

private:
  T makeReductionOutput(const Dim reductionDim) const;

  typename T::const_view_type m_data;
  GroupByGrouping m_grouping;
};

SCIPP_CORE_EXPORT GroupBy<DataArray> groupby(const DataConstProxy &dataset,
                                             const std::string &labels,
                                             const Dim targetDim);
SCIPP_CORE_EXPORT GroupBy<DataArray> groupby(const DataConstProxy &dataset,
                                             const std::string &labels,
                                             const VariableConstProxy &bins);

SCIPP_CORE_EXPORT GroupBy<Dataset> groupby(const DatasetConstProxy &dataset,
                                           const std::string &labels,
                                           const Dim targetDim);
SCIPP_CORE_EXPORT GroupBy<Dataset> groupby(const DatasetConstProxy &dataset,
                                           const std::string &labels,
                                           const VariableConstProxy &bins);

} // namespace scipp::core

#endif // SCIPP_CORE_GROUPBY_H
