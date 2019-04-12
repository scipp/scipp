// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <regex>

#include "dataset.h"
#include "dimensions.h"
#include "except.h"
#include "tags.h"

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

std::string do_to_string(const Tag tag) {
  switch (tag.value()) {
  case Coord::Monitor.value():
    return "Coord::Monitor";
  case Coord::DetectorInfo.value():
    return "Coord::DetectorInfo";
  case Coord::ComponentInfo.value():
    return "Coord::ComponentInfo";
  case Coord::X.value():
    return "Coord::X";
  case Coord::Y.value():
    return "Coord::Y";
  case Coord::Z.value():
    return "Coord::Z";
  case Coord::Qx.value():
    return "Coord::Qx";
  case Coord::Qy.value():
    return "Coord::Qy";
  case Coord::Qz.value():
    return "Coord::Qz";
  case Coord::Tof.value():
    return "Coord::Tof";
  case Coord::DSpacing.value():
    return "Coord::DSpacing";
  case Coord::Energy.value():
    return "Coord::Energy";
  case Coord::DeltaE.value():
    return "Coord::DeltaE";
  case Coord::Ei.value():
    return "Coord::Ei";
  case Coord::Ef.value():
    return "Coord::Ef";
  case Coord::DetectorId.value():
    return "Coord::DetectorId";
  case Coord::SpectrumNumber.value():
    return "Coord::SpectrumNumber";
  case Coord::DetectorGrouping.value():
    return "Coord::DetectorGrouping";
  case Coord::Row.value():
    return "Coord::Row";
  case Coord::Run.value():
    return "Coord::Run";
  case Coord::Polarization.value():
    return "Coord::Polarization";
  case Coord::Temperature.value():
    return "Coord::Temperature";
  case Coord::Time.value():
    return "Coord::Time";
  case Coord::Mask.value():
    return "Coord::Mask";
  case Coord::Position.value():
    return "Coord::Position";
  case Data::NoTag.value():
    return "Data::NoTag";
  case Data::Events.value():
    return "Data::Events";
  case Data::Value.value():
    return "Data::Value";
  case Data::Variance.value():
    return "Data::Variance";
  case Data::Tof.value():
    return "Data::Tof";
  case Data::PulseTime.value():
    return "Data::PulseTime";
  case Attr::ExperimentLog.value():
    return "Attr::ExperimentLog";
  default:
    return "<unknown tag>";
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

std::string to_string(const units::Unit &unit, const std::string &separator) {
  return std::regex_replace(do_to_string(unit), std::regex("::"), separator);
}
std::string to_string(const Dim dim, const std::string &separator) {
  return std::regex_replace(do_to_string(dim), std::regex("::"), separator);
}

std::string to_string(const Tag tag, const std::string &separator) {
  return std::regex_replace(do_to_string(tag), std::regex("::"), separator);
}

std::string make_dims_labels(const Variable &variable,
                             const std::string &separator,
                             const Dimensions &datasetDims) {
  const auto &dims = variable.dimensions();
  if (dims.empty())
    return "()";
  std::string diminfo = "(";
  for (const auto dim : dims.labels()) {
    if (datasetDims.contains(dim) && (datasetDims[dim] + 1 == dims[dim]))
      diminfo += "Bin-edges: ";
    diminfo += to_string(dim, separator);
    diminfo += ", ";
  }
  diminfo.resize(diminfo.size() - 2);
  diminfo += ")";
  return diminfo;
}

template <class Var>
auto to_string_components(const Var &variable, const std::string &separator,
                          const Dimensions &datasetDims = Dimensions()) {
  std::array<std::string, 5> out;
  out[0] = variable.name();
  out[1] = to_string(variable.tag(), separator);
  out[2] = to_string(variable.dtype());
  out[3] = '[' + variable.unit().name() + ']';
  out[4] = make_dims_labels(variable, separator, datasetDims);
  return out;
}

std::string format_name_and_tag(const std::string &name,
                                const std::string &tag) {
  if (name.empty())
    return '(' + tag + ')';
  return '(' + tag + ", " + name + ')';
}

void format_line(std::stringstream &s,
                 const std::array<std::string, 5> &columns) {
  const auto & [ name, tag, dtype, unit, dims ] = columns;
  const std::string tab("    ");
  const std::string colSep("  ");
  s << tab << std::left << std::setw(24) << format_name_and_tag(name, tag);
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
  for (const auto &var : dataset) {
    if (var.isCoord())
      format_line(s, to_string_components(var, separator, dims));
  }
  s << "Data:\n";
  for (const auto &var : dataset) {
    if (var.isData())
      format_line(s, to_string_components(var, separator, dims));
  }
  s << "Attributes:\n";
  for (const auto &var : dataset) {
    if (var.isAttr())
      format_line(s, to_string_components(var, separator, dims));
  }
  s << '\n';
  return s.str();
}

std::string to_string(const Dataset &dataset, const std::string &separator) {
  return do_to_string(dataset, "<Dataset>", dataset.dimensions(), separator);
}

std::string to_string(const ConstDatasetSlice &dataset,
                      const std::string &separator) {
  return do_to_string(dataset, "<DatasetSlice>", dataset.dimensions(),
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
DatasetError::DatasetError(const ConstDatasetSlice &dataset,
                           const std::string &message)
    : std::runtime_error(to_string(dataset) + message) {}

VariableNotFoundError::VariableNotFoundError(const Dataset &dataset,
                                             const Tag tag,
                                             const std::string &name)
    : DatasetError(dataset, "could not find variable with tag " +
                                to_string(tag) + " and name `" + name + "`.") {}
VariableNotFoundError::VariableNotFoundError(const ConstDatasetSlice &dataset,
                                             const Tag tag,
                                             const std::string &name)
    : DatasetError(dataset, "could not find variable with tag " +
                                to_string(tag) + " and name `" + name + "`.") {}
VariableNotFoundError::VariableNotFoundError(const Dataset &dataset,
                                             const std::string &name)
    : DatasetError(dataset,
                   "could not find any variable with name " + name + "`.") {}
VariableNotFoundError::VariableNotFoundError(const ConstDatasetSlice &dataset,
                                             const std::string &name)
    : DatasetError(dataset,
                   "could not find any variable with name " + name + "`.") {}

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
} // namespace expect
} // namespace scipp::core
