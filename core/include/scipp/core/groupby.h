// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_GROUPBY_H
#define SCIPP_CORE_GROUPBY_H

#include <vector>

#include <scipp/core/dataset.h>

namespace scipp::core {

/// Helper class for implementing "split-apply-combine" functionality.
class SCIPP_CORE_EXPORT GroupBy {
public:
  GroupBy(const DatasetConstProxy &data, Variable &&key,
          std::vector<std::vector<Slice>> &&groups)
      : m_data(data), m_key(std::move(key)), m_groups(std::move(groups)) {}

  scipp::index size() const noexcept { return scipp::size(m_groups); }
  Dim dim() const noexcept { return m_key.dims().inner(); }

  Dataset mean(const Dim reductionDim) const;
  Dataset sum(const Dim reductionDim) const;

private:
  Dataset makeReductionOutput(const Dim reductionDim) const;

  DatasetConstProxy m_data;
  Variable m_key;
  std::vector<std::vector<Slice>> m_groups;
};

SCIPP_CORE_EXPORT GroupBy groupby(const DatasetConstProxy &dataset,
                                  const std::string &labels,
                                  const Dim targetDim);

} // namespace scipp::core

#endif // SCIPP_CORE_GROUPBY_H
