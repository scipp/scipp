// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_UNITS_DLL_CONFIG_H
#define SCIPP_UNITS_DLL_CONFIG_H

#include "scipp/units/visibility.h"

#ifdef IN_SCIPP_UNITS
#define SCIPP_UNITS_DLL DLLExport
#define EXTERN_SCIPP_UNITS
#else
#define SCIPP_UNITS_DLL DLLImport
#define EXTERN_SCIPP_UNITS EXTERN_IMPORT
#endif

#endif // SCIPP_UNITS_DLL_CONFIG_H
