// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <string>

#include "scipp-dataset_export.h"
#include "scipp/core/string.h"
#include "scipp/dataset/sized_dict_forward.h"
#include "scipp/variable/string.h"

namespace scipp::dataset {

class Dataset;
class DataArray;

SCIPP_DATASET_EXPORT std::ostream &operator<<(std::ostream &os,
                                              const DataArray &data);
SCIPP_DATASET_EXPORT std::ostream &operator<<(std::ostream &os,
                                              const Dataset &dataset);

SCIPP_DATASET_EXPORT std::string to_string(const DataArray &data);
SCIPP_DATASET_EXPORT std::string to_string(const Dataset &dataset);

SCIPP_DATASET_EXPORT std::string to_string(const Coords &coords);
SCIPP_DATASET_EXPORT std::string to_string(const Masks &masks);
SCIPP_DATASET_EXPORT std::string
dict_keys_to_string(const CoordsHolder &coords);
SCIPP_DATASET_EXPORT std::string dict_keys_to_string(const Coords &coords);
SCIPP_DATASET_EXPORT std::string dict_keys_to_string(const Masks &masks);
SCIPP_DATASET_EXPORT std::string dict_keys_to_string(const Dataset &dataset);

} // namespace scipp::dataset
