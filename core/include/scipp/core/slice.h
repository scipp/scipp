// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#include "scipp-core_export.h"
#include "scipp/core/index.h"
#include "scipp/units/unit.h"

#ifndef SLICE_H
#define SLICE_H

namespace scipp {
namespace core {

/// Describes a slice to make over a dimension either as a single index or as a
/// range
class SCIPP_CORE_EXPORT Slice {

public:
  /// Constructor
  /// \param dim_ Slice Dimension
  /// \param begin_ start index or single index of the slice
  /// \param end_ end index for the range. Note that -1 indicates a point slice,
  /// not before-end.
  ///
  Slice(const Dim dim_, const scipp::index begin_, const scipp::index end_ = -1)
      : m_dim(dim_), m_begin(begin_), m_end(end_) {}
  Slice &operator=(const Slice &) = default;
  scipp::index begin() const { return m_begin; }
  scipp::index end() const { return m_end; }
  Dim dim() const { return m_dim; }

private:
  Dim m_dim;
  scipp::index m_begin;
  scipp::index m_end;
};

} // namespace core
} // namespace scipp

#endif // SLICE_H
