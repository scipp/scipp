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
  explicit Strides(const scipp::span<const scipp::index> &strides);
  Strides(std::initializer_list<scipp::index> strides);
  explicit Strides(const Dimensions &dims);

  bool operator==(const Strides &other) const noexcept;
  bool operator!=(const Strides &other) const noexcept;

  constexpr scipp::index operator[](const scipp::index i) const {
    return m_strides.at(i);
  }

  constexpr scipp::index &operator[](const scipp::index i) {
    return m_strides.at(i);
  }

  [[nodiscard]] auto begin() const { return m_strides.begin(); }
  [[nodiscard]] auto begin() { return m_strides.begin(); }
  [[nodiscard]] auto end(const scipp::index ndim) const {
    return std::next(m_strides.begin(), ndim);
  }
  [[nodiscard]] auto end(const scipp::index ndim) {
    return std::next(m_strides.begin(), ndim);
  }

  void erase(scipp::index i);

private:
  std::array<scipp::index, NDIM_MAX> m_strides{0};
};

Strides SCIPP_CORE_EXPORT transpose(const Strides &strides, Dimensions from,
                                    span<const Dim> order);

} // namespace scipp::core

namespace scipp {
using core::Strides;
}
