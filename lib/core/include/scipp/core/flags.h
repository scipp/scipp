// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-core_export.h"

namespace scipp {

enum class SCIPP_CORE_EXPORT CopyPolicy { Always, TryAvoid };

enum class SCIPP_CORE_EXPORT FillValue {
  Default,
  ZeroForSum,
  True,
  False,
  Max,
  Lowest
};

/// Fill value for elements masked out of the *input* of a reduction.
///
/// The value is substituted into the input before it is accumulated, so it must
/// preserve the input's dtype. ZeroForSum, which promotes bool and int32 to
/// int64, is therefore replaced by the equivalent zero of the input's own
/// dtype. The promotion applies to the accumulator only.
///
/// (ZeroForSum could instead be dropped from FillValue, making the promotion a
/// property of the reduction, which would compute the accumulator's dtype
/// itself. That is a larger change than the one this helper avoids.)
constexpr FillValue mask_fill_value(const FillValue fill) noexcept {
  return fill == FillValue::ZeroForSum ? FillValue::Default : fill;
}

enum class SCIPP_CORE_EXPORT SortOrder { Ascending, Descending };

} // namespace scipp
