// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_PARALLEL_H
#define SCIPP_CORE_PARALLEL_H

#include <algorithm>
#include <tbb/parallel_for.h>

#include "scipp/common/index.h"

/// Wrappers for multi-threading using TBB.
namespace scipp::core::parallel {

inline auto blocked_range(const scipp::index begin, const scipp::index end,
                          const scipp::index grainsize = -1) {
  // TBB's default grain-size is 1, which is probably quite inefficient in
  // some cases, in particular given the slow random-access of ViewIndex. A
  // good default value is not known right now. In practice this should also
  // depend heavily on whether we are processing small elements like `double`
  // or something large like `sparse_container<double>`.
  return tbb::blocked_range<scipp::index>(
      begin, end,
      grainsize == -1 ? std::max(scipp::index(1), (end - begin) / 24)
                      : grainsize);
}

template <class... Args> void parallel_for(Args &&... args) {
  if (auto *thread_limit = std::getenv("SCIPP_NUM_THREADS")) {
    tbb::task_arena limited_arena(atoi(thread_limit));
    limited_arena.execute(
        [&args...]() { tbb::parallel_for(std::forward<Args>(args)...); });
  } else
    tbb::parallel_for(std::forward<Args>(args)...);
}

} // namespace scipp::core::parallel

#endif // SCIPP_CORE_PARALLEL_H
