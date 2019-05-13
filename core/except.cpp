// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <regex>

#include "dataset.h"
#include "dimensions.h"
#include "except.h"

namespace scipp::core {

namespace {
std::string do_to_string(const Dim dim) {
  switch (dim) {
  case Dim::Invalid:
    return "<invalid>";
  case Dim::Event:
    return "Dim::Event";
  case Dim::Tof:
    return "Dim::Tof";
  case Dim::DSpacing:
    return "Dim::DSpacing";
  case Dim::Energy:
    return "Dim::Energy";
  case Dim::DeltaE:
    return "Dim::DeltaE";
  case Dim::Position:
    return "Dim::Position";
  case Dim::Spectrum:
    return "Dim::Spectrum";
  case Dim::Monitor:
    return "Dim::Monitor";
  case Dim::Run:
    return "Dim::Run";
  case Dim::Detector:
    return "Dim::Detector";
  case Dim::Q:
    return "Dim::Q";
  case Dim::X:
    return "Dim::X";
  case Dim::Y:
    return "Dim::Y";
  case Dim::Z:
    return "Dim::Z";
  case Dim::Qx:
    return "Dim::Qx";
  case Dim::Qy:
    return "Dim::Qy";
  case Dim::Qz:
    return "Dim::Qz";
  case Dim::Polarization:
    return "Dim::Polarization";
  case Dim::Temperature:
    return "Dim::Temperature";
  case Dim::Time:
    return "Dim::Time";
  case Dim::DetectorScan:
    return "Dim::DetectorScan";
  case Dim::Component:
    return "Dim::Component";
  case Dim::Row:
    return "Dim::Row";
  default:
    return "<unknown dimension>";
  }
}

std::string do_to_string(const units::Unit &unit) { return unit.name(); }
} // namespace

std::string to_string(const Dimensions &dims, const std::string &separator) {
  if (dims.empty())
    return "{}";
  std::string s = "{{";
  for (int32_t i = 0; i < dims.ndim(); ++i)
    s += to_string(dims.labels()[i], separator) + ", " +
         std::to_string(dims.shape()[i]) + "}, {";
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
  case DType::Char:
    return "char";
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
  case DType::EigenVector3d:
    return "Eigen::Vector3d";
  case DType::Unknown:
    return "unknown";
  default:
    return "unregistered dtype";
  };
}

std::string to_string(const Slice &slice, const std::string &separator) {
  std::string end = slice.end >= 0 ? ", " + std::to_string(slice.end) : "";
  return "Slice(" + to_string(slice.dim, separator) + ", " +
         std::to_string(slice.begin) + end + ")\n";
}

std::string to_string(const units::Unit &unit, const std::string &separator) {
  return std::regex_replace(do_to_string(unit), std::regex("::"), separator);
}
std::string to_string(const Dim dim, const std::string &separator) {
  return std::regex_replace(do_to_string(dim), std::regex("::"), separator);
}

std::string make_dims_labels(const Variable &variable,
                             const std::string &separator,
                             const Dimensions &datasetDims) {
  const auto &dims = variable.dims();
  if (dims.empty())
    return "()";
  std::string diminfo = "(";
  for (const auto dim : dims.labels()) {
    diminfo += to_string(dim, separator);
    if (datasetDims.contains(dim) && (datasetDims[dim] + 1 == dims[dim]))
      diminfo += " [bin-edges]";
    diminfo += ", ";
  }
  if (variable.isSparse()) {
    diminfo += to_string(variable.sparseDim(), separator);
    diminfo += " [sparse]";
    diminfo += ", ";
  }
  diminfo.resize(diminfo.size() - 2);
  diminfo += ")";
  return diminfo;
}

template <class Var>
auto to_string_components(const Var &variable, const std::string &separator,
                          const Dimensions &datasetDims = Dimensions()) {
  std::array<std::string, 3> out;
  out[0] = to_string(variable.dtype());
  out[1] = '[' + variable.unit().name() + ']';
  out[2] = make_dims_labels(variable, separator, datasetDims);
  return out;
}

auto &to_string(const std::string &s) { return s; }
auto to_string(const std::string_view s) { return s; }

template <class Key, class Var>
auto to_string_components(const Key &key, const Var &variable,
                          const std::string &separator,
                          const Dimensions &datasetDims = Dimensions()) {
  std::array<std::string, 4> out;
  out[0] = to_string(key);
  out[1] = to_string(variable.dtype());
  out[2] = '[' + variable.unit().name() + ']';
  out[3] = make_dims_labels(variable, separator, datasetDims);
  return out;
}

void format_line(std::stringstream &s,
                 const std::array<std::string, 4> &columns) {
  const auto & [ name, dtype, unit, dims ] = columns;
  const std::string tab("    ");
  const std::string colSep("  ");
  s << tab << std::left << std::setw(24) << name;
  s << colSep << std::setw(8) << dtype;
  s << colSep << std::setw(15) << unit;
  s << colSep << dims;
  s << '\n';
}

void format_line(std::stringstream &s,
                 const std::array<std::string, 3> &columns) {
  const auto & [ dtype, unit, dims ] = columns;
  const std::string colSep("  ");
  s << colSep << std::setw(8) << dtype;
  s << colSep << std::setw(15) << unit;
  s << colSep << dims;
  s << '\n';
}

std::string to_string(const Variable &variable, const std::string &separator) {
  std::stringstream s;
  s << "<Variable>";
  format_line(s, to_string_components(variable, separator));
  return s.str();
}

std::string to_string(const ConstVariableSlice &variable,
                      const std::string &separator) {
  std::stringstream s;
  s << "<VariableSlice>";
  format_line(s, to_string_components(variable, separator));
  return s.str();
}

template <class D>
std::string do_to_string(const D &dataset, const std::string &id,
                         const Dimensions &dims, const std::string &separator) {
  std::stringstream s;
  s << id + '\n';
  s << "Dimensions: " << to_string(dims, separator) << '\n';
  s << "Coordinates:\n";
  for (const auto & [ dim, var ] : dataset.coords())
    format_line(s, to_string_components(dim, var, separator, dims));
  for (const auto & [ name, var ] : dataset.labels())
    format_line(s, to_string_components(name, var, separator, dims));
  s << "Data:\n";
  for (const auto & [ name, var ] : dataset) {
    format_line(s, to_string_components(name, var.data(), separator, dims));
  }
  s << "Attributes:\n";
  for (const auto & [ name, var ] : dataset.attrs())
    format_line(s, to_string_components(name, var, separator, dims));
  s << '\n';
  return s.str();
}

template <class T> Dimensions dimensions(const T &dataset) {
  Dimensions datasetDims;
  // TODO Should probably include dimensions of coordinates and labels?
  for (const auto &item : dataset) {
    const auto &dims = item.second.dims();
    for (const auto dim : dims.labels())
      if (!datasetDims.contains(dim))
        datasetDims.add(dim, dims[dim]);
  }
  return datasetDims;
}

std::string to_string(const Dataset &dataset, const std::string &separator) {
  return do_to_string(dataset, "<Dataset>", dimensions(dataset), separator);
}

std::string to_string(const DatasetConstProxy &dataset,
                      const std::string &separator) {
  return do_to_string(dataset, "<DatasetSlice>", dimensions(dataset),
                      separator);
}

namespace except {

DimensionMismatchError::DimensionMismatchError(const Dimensions &expected,
                                               const Dimensions &actual)
    : DimensionError("Expected dimensions " + to_string(expected) + ", got " +
                     to_string(actual) + ".") {}

DimensionNotFoundError::DimensionNotFoundError(const Dimensions &expected,
                                               const Dim actual)
    : DimensionError("Expected dimension to be in " + to_string(expected) +
                     ", got " + to_string(actual) + ".") {}

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
VariableError::VariableError(const ConstVariableSlice &variable,
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

} // namespace expect
} // namespace scipp::core
