/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "except.h"
#include "dataset.h"
#include "dimensions.h"
#include "tags.h"
#include <regex>

namespace {
std::string do_to_string(const Dim dim) {
  switch (dim) {
  case Dim::Invalid:
    return "<invalid>";
  case Dim::Event:
    return "Dim::Event";
  case Dim::Tof:
    return "Dim::Tof";
  case Dim::Energy:
    return "Dim::Energy";
  case Dim::DeltaE:
    return "Dim::DeltaE";
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
  case Coord::Tof.value():
    return "Coord::Tof";
  case Coord::Energy.value():
    return "Coord::Energy";
  case Coord::DeltaE.value():
    return "Coord::DeltaE";
  case Coord::X.value():
    return "Coord::X";
  case Coord::Y.value():
    return "Coord::Y";
  case Coord::Z.value():
    return "Coord::Z";
  case Coord::SpectrumNumber.value():
    return "Coord::SpectrumNumber";
  case Coord::Mask.value():
    return "Coord::Mask";
  case Coord::Position.value():
    return "Coord::Position";
  case Coord::DetectorGrouping.value():
    return "Coord::DetectorGrouping";
  case Data::Value.value():
    return "Data::Value";
  case Data::Variance.value():
    return "Data::Variance";
  default:
    return "<unknown tag>";
  }
}

std::string do_to_string(const Unit &unit) {
  switch (unit.id()) {
  case Unit::Id::Dimensionless:
    return "Unit::Dimensionless";
  case Unit::Id::Length:
    return "Unit::Length";
  default:
    return "<unknown unit>";
  }
}
} // namespace

namespace dataset {

std::string to_string(const Dimensions &dims, const std::string &separator) {
  if (dims.empty())
    return "{}";
  std::string s = "{{";
  for (int32_t i = 0; i < dims.ndim(); ++i)
    s += to_string(dims.labels()[i], separator) + ", " +
         std::to_string(dims.shape()[i]) + "}, {";
  s.resize(s.size() - 3);
  s += "}\n";
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
    return "dataset";
  case DType::Float:
    return "float";
  case DType::Double:
    return "double";
  case DType::Int32:
    return "int32";
  case DType::Int64:
    return "int64";
  case DType::Unknown:
    return "unknown";
  default:
    return "unregistered dtype";
  };
}

std::string to_string(const Unit &unit, const std::string &separator) {
  return std::regex_replace(do_to_string(unit), std::regex("::"), separator);
}
std::string to_string(const Dim dim, const std::string &separator) {
  return std::regex_replace(do_to_string(dim), std::regex("::"), separator);
}

std::string to_string(const Tag tag, const std::string &separator) {
  return std::regex_replace(do_to_string(tag), std::regex("::"), separator);
}

// For use with variables
std::string make_dims_labels(const Variable &variable,
                             const std::string &separator) {
  auto dims = variable.dimensions();
  std::string diminfo = "( ";
  for (int32_t i = 0; i < dims.ndim(); ++i) {
    diminfo += to_string(dims.labels()[i], separator);
    if (i != dims.ndim() - 1) {
      diminfo += ", ";
    }
  }
  diminfo += " )";
  return diminfo;
}

std::string to_string(const Variable &variable, const std::string &separator) {
  std::string variableName = variable.name();
  std::string diminfo = make_dims_labels(variable, separator);
  if (variableName.empty())
    variableName = "''";
  std::string s = "Variable(";
  s += to_string(variable.tag(), separator) + ", " + variableName + "," +
       diminfo + ", " + to_string(variable.dtype()) + ")\n";
  return s;
} // namespace dataset

std::string to_string(const Dataset &dataset, const std::string &separator) {
  std::string s("Dataset with ");
  s += std::to_string(dataset.size()) + " variables\n";
  s += "Dimensions :\n " + to_string(dataset.dimensions(), separator);
  // The following is peformed to allow variables to be sorted into catagories
  // of coordinate, data and attribute as part of output.
  s += "Coordinate Variables :\n";
  for (const auto &var : dataset) {
    if (var.isCoord())
      s += to_string(var, separator);
  }
  s += "Data Variables :\n";
  for (const auto &var : dataset) {
    if (var.isData())
      s += to_string(var, separator);
  }
  s += "Attribute Variables :\n";
  for (const auto &var : dataset) {
    if (var.isAttr())
      s += to_string(var, separator);
  }
  return s;
} // namespace dataset

std::string to_string(const ConstDatasetSlice &dataset) {
  std::string s("Dataset slice with ");
  s += std::to_string(dataset.size()) + " variables";
  return s;
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
                                           const gsl::index length)
    : DimensionError("Expected dimension to be in " + to_string(expected) +
                     ", got " + to_string(actual) +
                     " with mismatching length " + std::to_string(length) +
                     ".") {}

DatasetError::DatasetError(const Dataset &dataset, const std::string &message)
    : std::runtime_error(to_string(dataset) + ", " + message) {}
DatasetError::DatasetError(const ConstDatasetSlice &dataset,
                           const std::string &message)
    : std::runtime_error(to_string(dataset) + ", " + message) {}

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

UnitMismatchError::UnitMismatchError(const Unit &a, const Unit &b)
    : UnitError("Expected " + to_string(a) + " to be equal to " + to_string(b) +
                ".") {}

} // namespace except

namespace expect {
void dimensionMatches(const Dimensions &dims, const Dim dim,
                      const gsl::index length) {
  if (dims[dim] != length)
    throw except::DimensionLengthError(dims, dim, length);
}

void equals(const Unit &a, const Unit &b) {
  if (!(a == b))
    throw except::UnitMismatchError(a, b);
}
} // namespace expect
} // namespace dataset
