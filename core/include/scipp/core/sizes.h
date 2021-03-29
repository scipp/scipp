// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <unordered_map>

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/slice.h"
#include "scipp/units/dim.h"

namespace scipp::core {

/// Sibling of class Dimensions, but unordered.
class SCIPP_CORE_EXPORT Sizes {
public:
  Sizes() = default;
  Sizes(const Dimensions &dims);
  Sizes(const std::unordered_map<Dim, scipp::index> &sizes) : m_sizes(sizes) {}

  bool operator==(const Sizes &other) const;
  bool operator!=(const Sizes &other) const;

  bool contains(const Dim dim) const noexcept {
    return m_sizes.count(dim) != 0;
  }

  auto begin() const { return m_sizes.begin(); }
  auto end() const { return m_sizes.end(); }

  void clear();

  scipp::index operator[](const Dim dim) const;
  void set(const Dim dim, const scipp::index size);
  void erase(const Dim dim);
  bool contains(const Dimensions &dims);
  Sizes slice(const Slice &params) const;

private:
  // TODO More efficient implementation without memory allocations.
  std::unordered_map<Dim, scipp::index> m_sizes;
};

[[nodiscard]] SCIPP_CORE_EXPORT Sizes merge(const Sizes &a, const Sizes &b);

SCIPP_CORE_EXPORT bool is_edges(const Sizes &sizes, const Dimensions &dims,
                                const Dim dim);

SCIPP_CORE_EXPORT std::string to_string(const Sizes &sizes);

} // namespace scipp::core

namespace scipp {
using core::Sizes;
}
