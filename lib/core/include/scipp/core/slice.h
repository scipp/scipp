// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/units/dim.h"

#pragma once

namespace scipp::core {

/// Describes a slice to make over a dimension either as a single index or as a
/// range
class SCIPP_CORE_EXPORT Slice {
public:
  Slice() : m_dim(Dim::None), m_begin(-1), m_end(-1), m_stride(1) {}
  Slice(const Dim dim_, const scipp::index begin_, const scipp::index end_,
        const scipp::index stride_ = 1);
  Slice(const Dim dim_, const scipp::index begin_);
  Slice(const Slice &) = default;
  Slice &operator=(const Slice &) = default;
  bool operator==(const Slice &other) const noexcept;
  bool operator!=(const Slice &other) const noexcept;
  scipp::index begin() const noexcept { return m_begin; }
  scipp::index end() const noexcept { return m_end; }
  Dim dim() const noexcept { return m_dim; }
  scipp::index stride() const noexcept { return m_stride; }
  bool isRange() const noexcept { return m_end != -1; }

private:
  Dim m_dim;
  scipp::index m_begin;
  scipp::index m_end;
  scipp::index m_stride;
};

} // namespace scipp::core

namespace scipp {
using core::Slice;
} // namespace scipp
