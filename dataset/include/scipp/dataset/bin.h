// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <scipp/dataset/dataset.h>

namespace scipp::dataset {

SCIPP_DATASET_EXPORT DataArray bin(const DataArray &array,
                                   const std::vector<Variable> &edges,
                                   const std::vector<Variable> &groups = {},
                                   const std::vector<Dim> &erase = {});

template <class Coords, class Masks, class Attrs>
SCIPP_DATASET_EXPORT DataArray bin(const Variable &data, const Coords &coords,
                                   const Masks &masks, const Attrs &attrs,
                                   const std::vector<Variable> &edges,
                                   const std::vector<Variable> &groups = {},
                                   const std::vector<Dim> &erase = {});

} // namespace scipp::dataset
