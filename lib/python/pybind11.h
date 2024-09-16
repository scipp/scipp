// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

// When a module is split into several compilation units, *all* compilation
// units must include the extra headers with type casters, otherwise we get ODR
// errors/warning. This header provides all pybind11 includes that we are using.

// Warnings are raised by eigen headers with gcc12
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include <pybind11/eigen.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
