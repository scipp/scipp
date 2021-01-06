// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm> // for std::sort
#include <set>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

namespace scipp::dataset {

std::ostream &operator<<(std::ostream &os, const DataArrayConstView &data) {
  return os << to_string(data);
}

std::ostream &operator<<(std::ostream &os, const DataArrayView &data) {
  return os << DataArrayConstView(data);
}

std::ostream &operator<<(std::ostream &os, const DataArray &data) {
  return os << DataArrayConstView(data);
}

std::ostream &operator<<(std::ostream &os, const DatasetConstView &dataset) {
  return os << to_string(dataset);
}

std::ostream &operator<<(std::ostream &os, const DatasetView &dataset) {
  return os << DatasetConstView(dataset);
}

std::ostream &operator<<(std::ostream &os, const Dataset &dataset) {
  return os << DatasetConstView(dataset);
}

constexpr const char *tab = "    ";

template <class D>
std::string do_to_string(const D &dataset, const std::string &id,
                         const Dimensions &dims, const std::string &shift = "");

template <class T> auto sorted(const T &map) {
  using core::to_string;
  std::vector<std::pair<std::string, VariableConstView>> elems;
  for (const auto &[dim, var] : map)
    elems.emplace_back(to_string(dim), var);
  std::sort(elems.begin(), elems.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
  return elems;
}

template <class Key>
auto format_data_view(const Key &name, const DataArrayConstView &data,
                      const Dimensions &datasetDims, const std::string &shift,
                      const bool inline_meta) {
  std::stringstream s;
  s << shift << format_variable(name, data.data(), datasetDims);

  const std::string header_shift = inline_meta ? shift : (shift + tab);
  const std::string data_shift = inline_meta ? shift : (header_shift + tab);
  if (!data.masks().empty()) {
    s << header_shift << "Masks:\n";
    for (const auto &[key, var] : sorted(data.masks()))
      s << data_shift << format_variable(key, var, datasetDims);
  }
  if (!data.attrs().empty()) {
    s << header_shift << "Attributes:\n";
    for (const auto &[key, var] : sorted(data.attrs()))
      s << data_shift << format_variable(key, var, datasetDims);
  }
  return s.str();
}

template <class D>
std::string do_to_string(const D &dataset, const std::string &id,
                         const Dimensions &dims, const std::string &shift) {
  std::stringstream s;
  if (!id.empty())
    s << shift << id + '\n';
  s << shift << "Dimensions: " << to_string(dims) << '\n';

  if (!dataset.coords().empty()) {
    s << shift << "Coordinates:\n";
    CoordsConstView map;
    if constexpr (std::is_same_v<D, DataArray> ||
                  std::is_same_v<D, DataArrayConstView>)
      map = dataset.coords();
    else
      map = dataset.coords();
    for (const auto &[name, var] : sorted(map))
      s << shift << format_variable(name, var, dims);
  }

  if constexpr (std::is_same_v<D, DataArray> ||
                std::is_same_v<D, DataArrayConstView>) {
    s << shift << "Data:\n"
      << format_data_view(dataset.name(), dataset, dims, shift, true);
  } else {
    if (!dataset.empty())
      s << shift << "Data:\n";
    std::set<std::string> sorted_items;
    for (const auto &item : dataset)
      sorted_items.insert(item.name());
    for (const auto &name : sorted_items)
      s << shift << format_data_view(name, dataset[name], dims, shift, false);
  }

  s << '\n';
  return s.str();
}

template <class T> Dimensions dimensions(const T &dataset) {
  Dimensions dims;
  for (const auto &[dim, size] : dataset.dimensions())
    dims.add(dim, size);
  return dims;
}

std::string to_string(const DataArray &data) {
  return do_to_string(data, "<scipp.DataArray>", data.dims());
}

std::string to_string(const DataArrayConstView &data) {
  return do_to_string(data, "<scipp.DataArrayView>", data.dims());
}

std::string to_string(const Dataset &dataset) {
  return do_to_string(dataset, "<scipp.Dataset>", dimensions(dataset));
}

std::string to_string(const DatasetConstView &dataset) {
  return do_to_string(dataset, "<scipp.DatasetView>", dimensions(dataset));
}

} // namespace scipp::dataset
