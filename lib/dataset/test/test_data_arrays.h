// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <gtest/gtest.h>

#include "scipp/dataset/data_array.h"

/// Create a data array with coord and masks.
///
/// Different but compatible arrays can be created using different seeds. The
/// seed does not affect coords to ensure that the produced arrays can be
/// inserted into the same dataset or be used in binary operations.
scipp::DataArray make_data_array_1d(int64_t seed = 0);
