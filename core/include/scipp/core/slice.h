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
  Slice(const Dim dim_, const scipp::index begin_, const scipp::index end_);
  Slice(const Dim dim_, const scipp::index begin_);
  Slice &operator=(const Slice &) = default;
  bool operator==(const Slice &other) const;
  bool operator!=(const Slice &other) const;
  scipp::index begin() const;
  scipp::index end() const;
  Dim dim() const;
  bool isRange() const;

private:
  Dim m_dim;
  scipp::index m_begin;
  scipp::index m_end;
};

} // namespace core
} // namespace scipp

#endif // SLICE_H
