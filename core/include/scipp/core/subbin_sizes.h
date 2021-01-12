// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <vector>

#include "scipp-core_export.h"
#include "scipp/common/index.h"

namespace scipp::core {

// instead of this helper class, we could also just use two variables for
// offset and sizes (the latter one a bin-variable), and implement custom sum
// and cumsum ops.
class SCIPP_CORE_EXPORT SubbinSizes {
public:
  SubbinSizes() = default;
  SubbinSizes(const scipp::index offset, std::vector<scipp::index> &&sizes);
  const auto &offset() const noexcept { return m_offset; }
  const auto &sizes() const noexcept { return m_sizes; }

private:
  scipp::index m_offset{0};
  // consider boost::small_vector?
  std::vector<scipp::index> m_sizes; // TODO can we avoid many small vecs?
};

[[nodiscard]] SCIPP_CORE_EXPORT bool operator==(const SubbinSizes &a,
                                                const SubbinSizes &b);

[[nodiscard]] SCIPP_CORE_EXPORT SubbinSizes operator+(const SubbinSizes &a,
                                                      const SubbinSizes &b);

} // namespace scipp::core
