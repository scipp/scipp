// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

// When a module is split into several compilation units, *all* compilation
// units must include the extra headers with type casters, otherwise we get ODR
// errors/warning. This header provides all nanobind includes that we are using.

#include <nanobind/nanobind.h>

// Must precede every other use of the types it provides casters for.
#include "pybind11_compat.h"

// Warnings are raised by eigen headers with gcc12
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include <nanobind/eigen/dense.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <nanobind/make_iterator.h>
#include <nanobind/ndarray.h>
#include <nanobind/operators.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/variant.h>
#include <nanobind/typing.h>
