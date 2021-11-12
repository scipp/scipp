// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <iosfwd>
#include <map>
#include <optional>
#include <string>

#include "scipp/units/unit.h"

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/core/bucket.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/dtype.h"
#include "scipp/core/slice.h"

namespace scipp::core {

SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const Dimensions &dims);

SCIPP_CORE_EXPORT const std::string &to_string(const std::string &s);
SCIPP_CORE_EXPORT std::string_view to_string(const std::string_view s);
SCIPP_CORE_EXPORT std::string to_string(const char *s);
SCIPP_CORE_EXPORT std::string to_string(const bool b);
SCIPP_CORE_EXPORT std::string to_string(const DType dtype);
SCIPP_CORE_EXPORT std::string to_string(const Dimensions &dims);
SCIPP_CORE_EXPORT std::string to_string(const Sizes &sizes);
SCIPP_CORE_EXPORT std::string to_string(const Slice &slice);
SCIPP_CORE_EXPORT std::string to_string(const scipp::index_pair &index);

// Format a time point according to ISO 8601 including sub-second precision
// depending on the unit.
// No timezone conversion is performed and the result does not show a
// timezone offset.
SCIPP_CORE_EXPORT std::string to_iso_date(const scipp::core::time_point &item,
                                          const units::Unit &unit);

/// Return the global dtype name registry instance
SCIPP_CORE_EXPORT std::map<DType, std::string> &dtypeNameRegistry();

} // namespace scipp::core
