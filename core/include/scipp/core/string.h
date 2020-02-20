// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_STRING_H
#define SCIPP_CORE_STRING_H

#include <string>

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/core/dtype.h"
#include "scipp/units/unit.h"

namespace scipp::core {

class DataArrayView;
class DataArrayConstView;
class DatasetView;
class DatasetConstView;
class Dataset;
class DataArray;
class Dimensions;
class Variable;
class VariableView;
class VariableConstView;
class Slice;
template <class Id, class Data> class ConstView;
template <class T> class MutableView;

SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os, const Dim dim);
SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const VariableConstView &variable);
SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const VariableView &variable);
SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const Variable &variable);
SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const DataArrayConstView &data);
SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const DataArrayView &data);
SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const DataArray &data);
SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const DatasetConstView &dataset);
SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const DatasetView &dataset);
SCIPP_CORE_EXPORT std::ostream &operator<<(std::ostream &os,
                                           const Dataset &dataset);

SCIPP_CORE_EXPORT std::string to_string(const bool b);
SCIPP_CORE_EXPORT std::string to_string(const DType dtype);
SCIPP_CORE_EXPORT std::string to_string(const Dimensions &dims);
SCIPP_CORE_EXPORT std::string to_string(const Slice &slice);
SCIPP_CORE_EXPORT std::string to_string(const Variable &variable);
SCIPP_CORE_EXPORT std::string to_string(const VariableConstView &variable);
SCIPP_CORE_EXPORT std::string to_string(const DataArray &data);
SCIPP_CORE_EXPORT std::string to_string(const DataArrayConstView &data);
SCIPP_CORE_EXPORT std::string to_string(const Dataset &dataset);
SCIPP_CORE_EXPORT std::string to_string(const DatasetConstView &dataset);

template <class Id, class Data>
std::string to_string(const ConstView<Id, Data> &view) {
  std::stringstream ss;

  for (const auto &[key, item] : view) {
    ss << "<scipp.ConstView> (" << key << "):\n" << to_string(item);
  }
  return ss.str();
}

template <class T> std::string to_string(const MutableView<T> &mutableView) {
  std::stringstream ss;

  for (const auto &[key, item] : mutableView) {
    ss << "<scipp.MutableView> (" << key << "):\n" << to_string(item);
  }
  return ss.str();
}

template <class T> std::string array_to_string(const T &arr);

template <class T> std::string element_to_string(const T &item) {
  using std::to_string;
  if constexpr (std::is_same_v<T, std::string>)
    return {'"' + item + "\", "};
  else if constexpr (std::is_same_v<T, bool>)
    return core::to_string(item) + ", ";
  else if constexpr (std::is_same_v<T, Eigen::Vector3d>)
    return {"(" + to_string(item[0]) + ", " + to_string(item[1]) + ", " +
            to_string(item[2]) + "), "};
  else if constexpr (is_sparse_v<T>)
    return array_to_string(item) + ", ";
  else if constexpr (std::is_same_v<T, DataArray>)
    return {"DataArray, "};
  else if constexpr (std::is_same_v<T, Dataset>)
    return {"Dataset, "};
  else
    return to_string(item) + ", ";
}

template <class T> std::string array_to_string(const T &arr) {
  const auto size = scipp::size(arr);
  if (size == 0)
    return std::string("[]");
  std::string s = "[";
  for (scipp::index i = 0; i < scipp::size(arr); ++i) {
    if (i == 2 && size > 4) {
      s += "..., ";
      i = size - 2;
    }
    s += element_to_string(arr[i]);
  }
  s.resize(s.size() - 2);
  s += "]";
  return s;
}
} // namespace scipp::core

#endif // SCIPP_CORE_STRING_H
