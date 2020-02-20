// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_UNITS_DIMENSION_H
#define SCIPP_UNITS_DIMENSION_H

#include <cstdint>

#include "scipp-units_export.h"

/// Macro to declare available dimension labels in a particular namespace.
///
/// The arguments passed to the macro are converted into enum labels. `Invalid`
/// is a reserved name and is automatically added to the list. This also defines
/// a `to_string` function for the defined enum.
#define SCIPP_UNITS_DECLARE_DIMENSIONS(...)                                    \
  enum class SCIPP_UNITS_EXPORT DimId : uint16_t { __VA_ARGS__, Invalid };     \
                                                                               \
  namespace detail2 {                                                          \
  constexpr const char *names = #__VA_ARGS__;                                  \
  constexpr auto ndim = static_cast<size_t>(DimId::Invalid);                   \
  }

#endif // SCIPP_UNITS_DIMENSION_H
