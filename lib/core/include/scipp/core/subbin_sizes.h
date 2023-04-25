// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <string>
#include <vector>

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/common/initialization.h"

namespace scipp::core {

/// Helper of `bin` for representing rows of a sparse subbin-size array.
class SCIPP_CORE_EXPORT SubbinSizes {
public:
  using container_type = std::vector<scipp::index>;
  SubbinSizes() = default;
  SubbinSizes(const scipp::index offset, container_type &&sizes);
  const auto &offset() const noexcept { return m_offset; }
  const auto &sizes() const noexcept { return m_sizes; }
  void operator=(const scipp::index value);
  SubbinSizes &operator+=(const SubbinSizes &other);
  SubbinSizes &operator-=(const SubbinSizes &other);

  SubbinSizes cumsum_exclusive() const;
  scipp::index sum() const;
  SubbinSizes &add_intersection(const SubbinSizes &other);
  void exclusive_scan(SubbinSizes &x);

private:
  scipp::index m_offset{0};
  container_type m_sizes;
};

[[nodiscard]] SCIPP_CORE_EXPORT bool operator==(const SubbinSizes &a,
                                                const SubbinSizes &b);

[[nodiscard]] SCIPP_CORE_EXPORT SubbinSizes operator+(const SubbinSizes &a,
                                                      const SubbinSizes &b);

[[nodiscard]] SCIPP_CORE_EXPORT SubbinSizes operator-(const SubbinSizes &a,
                                                      const SubbinSizes &b);

} // namespace scipp::core

namespace scipp {
// This is used by `sum` to initialize the output. By default we want a length-0
// size vector.
template <> struct zero_init<core::SubbinSizes> {
  static core::SubbinSizes value() { return core::SubbinSizes(); }
};

} // namespace scipp
