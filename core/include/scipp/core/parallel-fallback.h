// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_PARALLEL_H
#define SCIPP_CORE_PARALLEL_H

#include <tbb/parallel_for.h>

#include "scipp/common/index.h"

/// Wrappers for multi-threading using TBB.
namespace scipp::core::parallel {

using blocked_range = tbb::blocked_range<scipp::index>;

template <class... Args> void parallel_for(Args &&... args) {
  tbb::parallel_for(std::forward<Args>(args)...);
}

} // namespace scipp::core::parallel

#endif // SCIPP_CORE_PARALLEL_H
