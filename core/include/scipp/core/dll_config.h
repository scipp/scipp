// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_DLL_CONFIG_H
#define SCIPP_CORE_DLL_CONFIG_H

#include "scipp/units/visibility.h"

#ifdef scipp_core_EXPORTS
#define SCIPP_CORE_DLL DLLExport
#define EXTERN_SCIPP_CORE
#else
#define SCIPP_CORE_DLL DLLImport
#define EXTERN_SCIPP_CORE EXTERN_IMPORT
#endif

#endif // SCIPP_CORE_DLL_CONFIG_H
