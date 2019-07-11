// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_VISIBILITY_H
#define SCIPP_VISIBILITY_H

#ifdef _WIN32
#define DLLExport __declspec(dllexport)
#define DLLImport __declspec(dllimport)
#define EXTERN_IMPORT extern
#elif defined(__GNUC__)
#define DLLExport __attribute__((visibility("default")))
#define DLLImport
#define EXTERN_IMPORT extern
#else
#define DLLExport
#define DLLImport
#define EXTERN_IMPORT
#endif

#endif // SCIPP_VISIBILITY_H
