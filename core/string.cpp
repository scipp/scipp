// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <set>

#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/slice.h"
#include "scipp/core/string.h"
#include "scipp/core/tag_util.h"

namespace scipp::core {

std::ostream &operator<<(std::ostream &os, const Dim dim) {
  return os << to_string(dim);
}

std::ostream &operator<<(std::ostream &os, const VariableConstProxy &variable) {
  return os << to_string(variable);
}

std::ostream &operator<<(std::ostream &os, const VariableProxy &variable) {
  return os << VariableConstProxy(variable);
}

std::ostream &operator<<(std::ostream &os, const Variable &variable) {
  return os << VariableConstProxy(variable);
}

std::ostream &operator<<(std::ostream &os, const DataConstProxy &data) {
  return os << to_string(data);
}

std::ostream &operator<<(std::ostream &os, const DataProxy &data) {
  return os << DataConstProxy(data);
}

std::ostream &operator<<(std::ostream &os, const DataArray &data) {
  return os << DataConstProxy(data);
}

std::ostream &operator<<(std::ostream &os, const DatasetConstProxy &dataset) {
  return os << to_string(dataset);
}

std::ostream &operator<<(std::ostream &os, const DatasetProxy &dataset) {
  return os << DatasetConstProxy(dataset);
}

std::ostream &operator<<(std::ostream &os, const Dataset &dataset) {
  return os << DatasetConstProxy(dataset);
}

constexpr const char *tab = "    ";

std::string to_string(const Dimensions &dims) {
  if (dims.empty())
    return "{}";
  std::string s = "{{";
  for (int32_t i = 0; i < dims.shape().size(); ++i)
    s += to_string(dims.denseLabels()[i]) + ", " +
         std::to_string(dims.shape()[i]) + "}, {";
  if (dims.sparse())
    s += to_string(dims.sparseDim()) + ", [sparse]}, {";
  s.resize(s.size() - 3);
  s += "}";
  return s;
}

std::string to_string(const DType dtype) {
  switch (dtype) {
  case DType::String:
    return "string";
  case DType::Bool:
    return "bool";
  case DType::DataArray:
    return "DataArray";
  case DType::Dataset:
    return "Dataset";
  case DType::Float:
    return "float";
  case DType::Double:
    return "double";
  case DType::Int32:
    return "int32";
  case DType::Int64:
    return "int64";
  case DType::SparseFloat:
    return "sparse_float";
  case DType::SparseDouble:
    return "sparse_double";
  case DType::SparseInt64:
    return "sparse_int64";
  case DType::SparseInt32:
    return "sparse_int32";
  case DType::EigenVector3d:
    return "vector_3_double";
  case DType::PyObject:
    return "PyObject";
  case DType::Unknown:
    return "unknown";
  default:
    return "unregistered dtype";
  };
}

std::string to_string(const Slice &slice) {
  std::string end = slice.end() >= 0 ? ", " + std::to_string(slice.end()) : "";
  return "Slice(" + to_string(slice.dim()) + ", " +
         std::to_string(slice.begin()) + end + ")\n";
}

std::string make_dims_labels(const VariableConstProxy &variable,
                             const Dimensions &datasetDims) {
  const auto &dims = variable.dims();
  if (dims.empty())
    return "()";
  std::string diminfo = "(";
  for (const auto dim : dims.denseLabels()) {
    diminfo += to_string(dim);
    if (datasetDims.contains(dim) && (datasetDims[dim] + 1 == dims[dim]))
      diminfo += " [bin-edges]";
    diminfo += ", ";
  }
  if (variable.dims().sparse()) {
    diminfo += to_string(variable.dims().sparseDim());
    diminfo += " [sparse]";
    diminfo += ", ";
  }
  diminfo.resize(diminfo.size() - 2);
  diminfo += ")";
  return diminfo;
}

auto &to_string(const std::string &s) { return s; }
auto to_string(const std::string_view s) { return s; }
auto to_string(const char *s) { return std::string(s); }

template <class T> struct ValuesToString {
  static auto apply(const VariableConstProxy &var) {
    return array_to_string(var.template values<T>());
  }
};
template <class T> struct VariancesToString {
  static auto apply(const VariableConstProxy &var) {
    return array_to_string(var.template variances<T>());
  }
};

template <class Key, class Var>
auto format_variable(const Key &key, const Var &variable,
                     const Dimensions &datasetDims = Dimensions()) {
  std::stringstream s;
  const std::string colSep("  ");
  s << tab << std::left << std::setw(24) << to_string(key);
  s << colSep << std::setw(9) << to_string(variable.dtype());
  s << colSep << std::setw(15) << '[' + variable.unit().name() + ']';
  s << colSep << make_dims_labels(variable, datasetDims);
  s << colSep
    << apply<ValuesToString>(variable.data().dtype(),
                             VariableConstProxy(variable));
  if (variable.hasVariances())
    s << colSep
      << apply<VariancesToString>(variable.data().dtype(),
                                  VariableConstProxy(variable));
  s << '\n';
  return s.str();
}

template <class Key>
auto format_data_proxy(const Key &name, const DataConstProxy &data,
                       const Dimensions &datasetDims = Dimensions()) {
  std::stringstream s;
  if (data.hasData())
    s << format_variable(name, data.data(), datasetDims);
  else
    s << tab << name << '\n';
  for (const auto &[dim, coord] : data.coords())
    if (coord.dims().sparse()) {
      s << tab << tab << "Sparse coordinate:\n";
      s << format_variable(std::string(tab) + std::string(tab) + to_string(dim),
                           coord, datasetDims);
    }
  bool sparseLabels = false;
  for (const auto &[label_name, labels] : data.labels())
    if (labels.dims().sparse()) {
      if (!sparseLabels) {
        s << tab << tab << "Sparse labels:\n";
        sparseLabels = true;
      }
      s << format_variable(std::string(tab) + std::string(tab) +
                               std::string(label_name),
                           labels, datasetDims);
    }
  return s.str();
}

std::string to_string(const Variable &variable) {
  return format_variable("<scipp.Variable>", variable);
}

std::string to_string(const VariableConstProxy &variable) {
  return format_variable("<scipp.VariableProxy>", variable);
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
  if (!dataset.labels().empty()) {
    s << "Labels:\n";
    for (const auto &[name, var] : dataset.labels())
      s << format_variable(name, var, dims);
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
                std::is_same_v<D, DataConstProxy>) {
    s << "Data:\n" << format_data_proxy(dataset.name(), dataset);
  } else {
    if (!dataset.empty())
      s << "Data:\n";
    std::set<std::string> sorted_items;
    for (const auto &item : dataset)
      sorted_items.insert(item.first);
    for (const auto &name : sorted_items)
      s << format_data_proxy(name, dataset[name], dims);
  }

  s << '\n';
  return s.str();
}

template <class T> Dimensions dimensions(const T &dataset) {
  Dimensions datasetDims;
  Dim sparse = Dim::Invalid;
  for (const auto &item : dataset) {
    const auto &dims = item.second.dims();
    for (const auto dim : dims.labels()) {
      if (!datasetDims.contains(dim)) {
        if (dim == dims.sparseDim())
          sparse = dim;
        else
          datasetDims.add(dim, dims[dim]);
      }
    }
  }
  for (const auto &coord : dataset.coords()) {
    const auto &dims = coord.second.dims();
    for (const auto dim : dims.labels())
      if (!datasetDims.contains(dim))
        datasetDims.add(dim, dims[dim]);
  }
  for (const auto &labels : dataset.labels()) {
    const auto &dims = labels.second.dims();
    for (const auto dim : dims.labels())
      if (!datasetDims.contains(dim))
        datasetDims.add(dim, dims[dim]);
  }
  if (sparse != Dim::Invalid && !datasetDims.contains(sparse))
    datasetDims.addInner(sparse, Dimensions::Sparse);
  return datasetDims;
}

std::string to_string(const DataArray &data) {
  return do_to_string(data, "<scipp.DataArray>", data.dims());
}

std::string to_string(const DataConstProxy &data) {
  return do_to_string(data, "<scipp.DataProxy>", data.dims());
}

std::string to_string(const Dataset &dataset) {
  return do_to_string(dataset, "<scipp.Dataset>", dimensions(dataset));
}

std::string to_string(const DatasetConstProxy &dataset) {
  return do_to_string(dataset, "<scipp.DatasetProxy>", dimensions(dataset));
}
} // namespace scipp::core
