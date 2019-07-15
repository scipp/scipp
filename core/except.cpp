// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/except.h"
#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/tag_util.h"

namespace scipp::core {

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
  case DType::EigenVector3d:
    return "Eigen::Vector3d";
  case DType::Unknown:
    return "unknown";
  default:
    return "unregistered dtype";
  };
}

std::string to_string(const Slice &slice) {
  std::string end = slice.end >= 0 ? ", " + std::to_string(slice.end) : "";
  return "Slice(" + to_string(slice.dim) + ", " + std::to_string(slice.begin) +
         end + ")\n";
}

std::string to_string(const units::Unit &unit) { return unit.name(); }

std::string make_dims_labels(const Variable &variable,
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
  s << colSep << std::setw(8) << to_string(variable.dtype());
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
  for (const auto & [ dim, coord ] : data.coords())
    if (coord.dims().sparse()) {
      s << tab << tab << "Sparse coordinate:\n";
      s << format_variable(std::string(tab) + std::string(tab) + to_string(dim),
                           coord, datasetDims);
    }
  bool sparseLabels = false;
  for (const auto & [ label_name, labels ] : data.labels())
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

std::string to_string(const DataConstProxy &data) {
  return format_data_proxy("<scipp.DataProxy>", data);
}

template <class D>
std::string do_to_string(const D &dataset, const std::string &id,
                         const Dimensions &dims) {
  std::stringstream s;
  s << id + '\n';
  s << "Dimensions: " << to_string(dims) << '\n';
  s << "Coordinates:\n";
  for (const auto & [ dim, var ] : dataset.coords())
    s << format_variable(dim, var, dims);
  s << "Labels:\n";
  for (const auto & [ name, var ] : dataset.labels())
    s << format_variable(name, var, dims);
  s << "Data:\n";
  for (const auto & [ name, item ] : dataset)
    s << format_data_proxy(name, item, dims);
  s << "Attributes:\n";
  for (const auto & [ name, var ] : dataset.attrs())
    s << format_variable(name, var, dims);
  s << '\n';
  return s.str();
}

template <class T> Dimensions dimensions(const T &dataset) {
  Dimensions datasetDims;
  Dim sparse = Dim::Invalid;
  // TODO Should probably include dimensions of coordinates and labels?
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
  if (sparse != Dim::Invalid && !datasetDims.contains(sparse))
    datasetDims.addInner(sparse, Dimensions::Sparse);
  return datasetDims;
}

std::string to_string(const Dataset &dataset) {
  return do_to_string(dataset, "<scipp.Dataset>", dimensions(dataset));
}

std::string to_string(const DatasetConstProxy &dataset) {
  return do_to_string(dataset, "<scipp.DatasetProxy>", dimensions(dataset));
}

namespace except {

DimensionMismatchError::DimensionMismatchError(const Dimensions &expected,
                                               const Dimensions &actual)
    : DimensionError("Expected dimensions " + to_string(expected) + ", got " +
                     to_string(actual) + ".") {}

DimensionNotFoundError::DimensionNotFoundError(const Dimensions &expected,
                                               const Dim actual)
    : DimensionError("Expected dimension to be a non-sparse dimension of " +
                     to_string(expected) + ", got " + to_string(actual) + ".") {
}

DimensionLengthError::DimensionLengthError(const Dimensions &expected,
                                           const Dim actual,
                                           const scipp::index length)
    : DimensionError("Expected dimension to be in " + to_string(expected) +
                     ", got " + to_string(actual) +
                     " with mismatching length " + std::to_string(length) +
                     ".") {}

DatasetError::DatasetError(const Dataset &dataset, const std::string &message)
    : std::runtime_error(to_string(dataset) + message) {}
DatasetError::DatasetError(const DatasetConstProxy &dataset,
                           const std::string &message)
    : std::runtime_error(to_string(dataset) + message) {}

VariableError::VariableError(const Variable &variable,
                             const std::string &message)
    : std::runtime_error(to_string(variable) + message) {}
VariableError::VariableError(const VariableConstProxy &variable,
                             const std::string &message)
    : std::runtime_error(to_string(variable) + message) {}

UnitMismatchError::UnitMismatchError(const units::Unit &a, const units::Unit &b)
    : UnitError("Expected " + to_string(a) + " to be equal to " + to_string(b) +
                ".") {}

} // namespace except

namespace expect {
void dimensionMatches(const Dimensions &dims, const Dim dim,
                      const scipp::index length) {
  if (dims[dim] != length)
    throw except::DimensionLengthError(dims, dim, length);
}

void equals(const units::Unit &a, const units::Unit &b) {
  if (!(a == b))
    throw except::UnitMismatchError(a, b);
}

void equals(const Dimensions &a, const Dimensions &b) {
  if (!(a == b))
    throw except::DimensionMismatchError(a, b);
}

void validSlice(const Dimensions &dims, const Slice &slice) {
  if (!dims.contains(slice.dim) || slice.begin < 0 ||
      slice.begin >= std::min(slice.end >= 0 ? slice.end + 1 : dims[slice.dim],
                              dims[slice.dim]) ||
      slice.end > dims[slice.dim])
    throw except::SliceError("Expected " + to_string(slice) + " to be in " +
                             to_string(dims) + ".");
}

void coordsAndLabelsMatch(const DataConstProxy &a, const DataConstProxy &b) {
  if (a.coords() != b.coords() || a.labels() != b.labels())
    throw except::CoordMismatchError("Expected coords and labels to match.");
}

void coordsAndLabelsAreSuperset(const DataConstProxy &a,
                                const DataConstProxy &b) {
  for (const auto & [ dim, coord ] : b.coords())
    if (a.coords()[dim] != coord)
      throw except::CoordMismatchError("Expected coords to match.");
  for (const auto & [ name, labels ] : b.labels())
    if (a.labels()[name] != labels)
      throw except::CoordMismatchError("Expected labels to match.");
}

void notSparse(const Dimensions &dims) {
  if (dims.sparse())
    throw except::DimensionError("Expected non-sparse dimensions.");
}

void validDim(const Dim dim) {
  if (dim == Dim::Invalid)
    throw except::DimensionError("Dim::Invalid is not a valid dimension.");
}

void validExtent(const scipp::index size) {
  if (size == Dimensions::Sparse)
    throw except::DimensionError("Expected non-sparse dimension extent.");
  if (size < 0)
    throw except::DimensionError("Dimension size cannot be negative.");
}

} // namespace expect
} // namespace scipp::core
