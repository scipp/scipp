// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-core_export.h"

namespace scipp {

enum class SCIPP_CORE_EXPORT CopyPolicy { Always, TryAvoid };

enum class SCIPP_CORE_EXPORT FillValue {
  Default,
  ZeroNotBool,
  True,
  False,
  Max,
  Lowest
};

enum class SCIPP_CORE_EXPORT SortOrder { Ascending, Descending };

} // namespace scipp
