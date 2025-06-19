// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <boost/container/small_vector.hpp>

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
  explicit Strides(const std::span<const scipp::index> &strides);
  Strides(std::initializer_list<scipp::index> strides);
  explicit Strides(const Dimensions &dims);

  bool operator==(const Strides &other) const noexcept;
  bool operator!=(const Strides &other) const noexcept;

  [[nodiscard]] scipp::index size() const noexcept { return m_strides.size(); }

  void push_back(const scipp::index i);
  void clear();
  void resize(const scipp::index size);

  scipp::index operator[](const scipp::index i) const {
    return m_strides.at(i);
  }

  scipp::index &operator[](const scipp::index i) { return m_strides.at(i); }

  [[nodiscard]] auto begin() const { return m_strides.begin(); }
  [[nodiscard]] auto begin() { return m_strides.begin(); }
  [[nodiscard]] auto end() const { return m_strides.end(); }
  [[nodiscard]] auto end() { return m_strides.end(); }

  void erase(scipp::index i);

private:
  boost::container::small_vector<scipp::index, NDIM_STACK> m_strides;
};

Strides SCIPP_CORE_EXPORT transpose(const Strides &strides, Dimensions from,
                                    std::span<const Dim> order);

} // namespace scipp::core

namespace scipp {
using core::Strides;
}
