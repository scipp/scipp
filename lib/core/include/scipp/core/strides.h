// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/core/dimensions.h"

namespace scipp::core {

/// Container for strides in memory.
///
/// Given a contiguous buffer in memory viewed as a multi-dimensional array,
/// `strides[d]` is the distance in memory between consecutive array elements
/// in dimension `d`.
/// @see scipp::flat_index_from_strides
///
/// Strides objects do not store information on the dimensions.
/// They should therefore be accompanied by an instance of Dimensions.
/// The storage order in Strides is arbitrary but should be the same as in
/// Dimensions, i.e. the slowest moving index should be first.
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

  [[nodiscard]] constexpr auto begin() const { return m_strides.begin(); }
  [[nodiscard]] constexpr auto begin() { return m_strides.begin(); }
  [[nodiscard]] constexpr auto end(const scipp::index ndim) const {
    return std::next(m_strides.begin(), ndim);
  }
  [[nodiscard]] constexpr auto end(const scipp::index ndim) {
    return std::next(m_strides.begin(), ndim);
  }

  void erase(scipp::index i);

private:
  std::array<scipp::index, NDIM_MAX> m_strides{0};
};

Strides SCIPP_CORE_EXPORT transpose(const Strides &strides, Dimensions from,
                                    scipp::span<const Dim> order);

} // namespace scipp::core

namespace scipp {
using core::Strides;
}
