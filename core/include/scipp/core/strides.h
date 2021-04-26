// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/common/span.h"
#include "scipp/core/dimensions.h"

namespace scipp::core {

class SCIPP_CORE_EXPORT Strides {
public:
  Strides() = default;
  Strides(const scipp::span<const scipp::index> &strides);
  Strides(const Dimensions &dims);

  bool operator==(const Strides &other) const noexcept;
  bool operator!=(const Strides &other) const noexcept;

  scipp::index operator[](const scipp::index i) const {
    return m_strides.at(i);
  }

  auto begin() const { return m_strides.begin(); }

  void erase(const scipp::index i);

private:
  std::array<scipp::index, NDIM_MAX> m_strides{0};
};

Strides SCIPP_CORE_EXPORT transpose(const Strides &strides, Dimensions from,
                                    span<const Dim> order);

} // namespace scipp::core

namespace scipp {
using core::Strides;
}
