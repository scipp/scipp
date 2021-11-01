// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>

#include "scipp/common/index.h"

/// Fallback wrappers without actual threading, in case TBB is not available.
namespace scipp::core::parallel {

class blocked_range {
public:
  constexpr blocked_range(const scipp::index begin, const scipp::index end,
                          const scipp::index grainsize = 1) noexcept
      : m_begin(begin), m_end(end) {
    static_cast<void>(grainsize);
  }
  constexpr scipp::index begin() const noexcept { return m_begin; }
  constexpr scipp::index end() const noexcept { return m_end; }

private:
  scipp::index m_begin;
  scipp::index m_end;
};

template <class Op> void parallel_for(const blocked_range &range, Op &&op) {
  op(range);
}

template <class... Args> void parallel_sort(Args &&... args) {
  std::sort(std::forward<Args>(args)...);
}

} // namespace scipp::core::parallel
