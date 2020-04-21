// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <string>

#include "scipp-dataset_export.h"
#include "scipp/core/string.h"
#include "scipp/dataset/map_view_forward.h"
#include "scipp/variable/string.h"

namespace scipp::dataset {

class DataArrayView;
class DataArrayConstView;
class DatasetView;
class DatasetConstView;
class Dataset;
class DataArray;
template <class Id, class Key, class Value> class ConstView;
template <class T, class U> class MutableView;

SCIPP_DATASET_EXPORT std::ostream &operator<<(std::ostream &os,
                                              const DataArrayConstView &data);
SCIPP_DATASET_EXPORT std::ostream &operator<<(std::ostream &os,
                                              const DataArrayView &data);
SCIPP_DATASET_EXPORT std::ostream &operator<<(std::ostream &os,
                                              const DataArray &data);
SCIPP_DATASET_EXPORT std::ostream &operator<<(std::ostream &os,
                                              const DatasetConstView &dataset);
SCIPP_DATASET_EXPORT std::ostream &operator<<(std::ostream &os,
                                              const DatasetView &dataset);
SCIPP_DATASET_EXPORT std::ostream &operator<<(std::ostream &os,
                                              const Dataset &dataset);

SCIPP_DATASET_EXPORT std::string to_string(const DataArray &data);
SCIPP_DATASET_EXPORT std::string to_string(const DataArrayConstView &data);
SCIPP_DATASET_EXPORT std::string to_string(const Dataset &dataset);
SCIPP_DATASET_EXPORT std::string to_string(const DatasetConstView &dataset);

template <class Id, class Key, class Value>
std::string to_string(const ConstView<Id, Key, Value> &view) {
  std::stringstream ss;
  for (const auto &[key, item] : view) {
    ss << "<scipp.ConstView> (" << key << "):\n" << to_string(item);
  }
  return ss.str();
}

template <class T, class U>
std::string to_string(const MutableView<T, U> &mutableView) {
  std::stringstream ss;
  for (const auto &[key, item] : mutableView) {
    ss << "<scipp.MutableView> (" << key << "):\n" << to_string(item);
  }
  return ss.str();
}

} // namespace scipp::dataset
