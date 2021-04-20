// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <string>

#include "scipp-dataset_export.h"
#include "scipp/core/string.h"
#include "scipp/dataset/map_view_forward.h"
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

template <class Key, class Value>
std::string to_string(const Dict<Key, Value> &view) {
  std::stringstream ss;
  ss << "<scipp.Dict>\n";
  for (const auto &[key, item] : view) {
    ss << "  " << key << ":" << to_string(item);
  }
  return ss.str();
}

} // namespace scipp::dataset
