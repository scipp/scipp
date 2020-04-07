// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <set>

#include "scipp/dataset/dataset.h"

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

template <class Key>
auto format_data_view(const Key &name, const DataArrayConstView &data,
                      const Dimensions &datasetDims = Dimensions()) {
  std::stringstream s;
  if (data.hasData())
    s << format_variable(name, data.data(), datasetDims);
  else
    s << tab << name << '\n';
  // TODO need something similar, but for unaligned coord content
  /*
  bool sparseCoords = false;
  for (const auto &[dim, coord] : data.coords())
    if (coord.dims().sparse()) {
      if (!sparseCoords) {
        s << tab << tab << "Sparse coords:\n";
        sparseCoords = true;
      }
      s << format_variable(std::string(tab) + std::string(tab) + to_string(dim),
                           coord.data(), datasetDims);
    }
    */

  if (!data.attrs().empty()) {
    s << tab << "Attributes:\n";
    for (const auto &[attr_name, var] : data.attrs())
      s << tab << tab << format_variable(attr_name, var, datasetDims);
  }
  return s.str();
}

template <class D>
std::string do_to_string(const D &dataset, const std::string &id,
                         const Dimensions &dims) {
  std::stringstream s;
  s << id + '\n';
  s << "Dimensions: " << to_string(dims) << '\n';

  if (!dataset.coords().empty()) {
    s << "Coordinates:\n";
    for (const auto &[dim, var] : dataset.coords())
      s << format_variable(dim, var, dims);
  }
  if (!dataset.attrs().empty()) {
    s << "Attributes:\n";
    for (const auto &[name, var] : dataset.attrs())
      s << format_variable(name, var, dims);
  }
  if (!dataset.masks().empty()) {
    s << "Masks:\n";
    for (const auto &[name, var] : dataset.masks())
      s << format_variable(name, var, dims);
  }

  if constexpr (std::is_same_v<D, DataArray> ||
                std::is_same_v<D, DataArrayConstView>) {
    s << "Data:\n" << format_data_view(dataset.name(), dataset);
  } else {
    if (!dataset.empty())
      s << "Data:\n";
    std::set<std::string> sorted_items;
    for (const auto &item : dataset)
      sorted_items.insert(item.name());
    for (const auto &name : sorted_items)
      s << format_data_view(name, dataset[name], dims);
  }

  s << '\n';
  return s.str();
}

template <class T> Dimensions dimensions(const T &dataset) {
  Dimensions datasetDims;
  for (const auto &item : dataset) {
    const auto &dims = item.dims();
    for (const auto dim : dims.labels())
      if (!datasetDims.contains(dim))
        datasetDims.add(dim, dims[dim]);
  }
  for (const auto &coord : dataset.coords()) {
    const auto &dims = coord.second.dims();
    for (const auto dim : dims.labels())
      if (!datasetDims.contains(dim))
        datasetDims.add(dim, dims[dim]);
  }
  return datasetDims;
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
