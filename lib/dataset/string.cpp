// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm> // for std::sort
#include <iomanip>
#include <set>
#include <sstream>

#include "scipp/core/dict.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/string.h"

namespace scipp::dataset {

std::ostream &operator<<(std::ostream &os, const DataArray &data) {
  return os << to_string(data);
}

std::ostream &operator<<(std::ostream &os, const Dataset &dataset) {
  return os << to_string(dataset);
}

constexpr const char *tab = "  ";
constexpr const char *tab_minus_1 = " ";

template <class T> auto sorted(const T &map) {
  using core::to_string;
  std::vector<std::pair<std::string, Variable>> elems;
  for (const auto &[dim, var] : map)
    elems.emplace_back(to_string(dim), var);
  std::sort(elems.begin(), elems.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
  return elems;
}

namespace {
std::string format_variable(const std::string &key, const Variable &variable,
                            const std::optional<Sizes> &datasetSizes,
                            const bool show_alignment = false) {
  std::stringstream s;
  s << (show_alignment && variable.is_aligned() ? '*' : ' ') << tab_minus_1
    << std::left << std::setw(24) << key
    << format_variable(variable, datasetSizes) << '\n';
  return s.str();
}

template <class Key>
auto format_data_view(const Key &name, const DataArray &data,
                      const Sizes &datasetSizes, const std::string &shift,
                      const bool inline_meta) {
  std::stringstream s;
  s << shift << format_variable(name, data.data(), datasetSizes);

  const std::string header_shift = inline_meta ? shift : (shift + tab + tab);
  const std::string data_shift = inline_meta ? shift : (header_shift + tab);
  if (!data.masks().empty()) {
    s << header_shift << "Masks:\n";
    for (const auto &[key, var] : sorted(data.masks()))
      s << data_shift << format_variable(key, var, datasetSizes);
  }
  return s.str();
}

template <class D>
std::string do_to_string(const D &dataset, const std::string &id,
                         const Sizes &dims, const std::string &shift = "") {
  std::stringstream s;
  if (!id.empty())
    s << shift << id + '\n';
  s << shift << "Dimensions: " << to_string(dims) << '\n';

  if (!dataset.coords().empty()) {
    s << shift << "Coordinates:\n";
    const Coords &map = dataset.coords();
    for (const auto &[name, var] : sorted(map))
      s << shift << format_variable(name, var, dims, true);
  }

  if constexpr (std::is_same_v<D, DataArray>) {
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
} // namespace

std::string to_string(const DataArray &data) {
  return do_to_string(data, "<scipp.DataArray>", data.dims());
}

std::string to_string(const Dataset &dataset) {
  return do_to_string(dataset, "<scipp.Dataset>", dataset.sizes());
}

namespace {
template <class Key, class Value>
std::string dict_to_string(const SizedDict<Key, Value> &view) {
  std::stringstream ss;
  ss << "<scipp.Dict>\n";
  for (const auto &[key, item] : view)
    ss << "  " << key << ": " << to_string(item) << "\n";
  return ss.str();
}
} // namespace

std::string to_string(const Coords &coords) { return dict_to_string(coords); }
std::string to_string(const Masks &masks) { return dict_to_string(masks); }

std::string dict_keys_to_string(const Coords &coords) {
  return core::dict_keys_to_string(coords.keys_begin(), coords.keys_end(),
                                   "scipp.Dict.keys");
}

std::string dict_keys_to_string(const Masks &masks) {
  return core::dict_keys_to_string(masks.keys_begin(), masks.keys_end(),
                                   "scipp.Dict.keys");
}

std::string dict_keys_to_string(const Dataset &dataset) {
  return core::dict_keys_to_string(dataset.keys_begin(), dataset.keys_end(),
                                   "scipp.Dataset.keys");
}

} // namespace scipp::dataset
