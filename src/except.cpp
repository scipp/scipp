#include "except.h"
#include "dataset.h"
#include "dimensions.h"
#include "tags.h"

namespace dataset {

std::string to_string(const Dim dim) {
  switch (dim) {
  case Dim::Invalid:
    return "<invalid>";
  case Dim::Event:
    return "Dim::Event";
  case Dim::Tof:
    return "Dim::Tof";
  case Dim::MonitorTof:
    return "Dim::MonitorTof";
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

std::string to_string(const Tag tag) {
  switch (tag.value()) {
  case Coord::Tof{}.value():
    return "Coord::Tof";
  case Coord::X{}.value():
    return "Coord::X";
  case Coord::Y{}.value():
    return "Coord::Y";
  case Coord::Z{}.value():
    return "Coord::Z";
  case Data::Value{}.value():
    return "Data::Value";
  case Data::Variance{}.value():
    return "Data::Variance";
  case Data::Int{}.value():
    return "Data::Int";
  default:
    return "<unknown tag>";
  }
}

std::string to_string(const Dimensions &dims) {
  if (dims.empty())
    return "{}";
  std::string s = "{{";
  for (int32_t i = 0; i < dims.ndim(); ++i)
    s += to_string(dims.labels()[i]) + ", " + std::to_string(dims.shape()[i]) +
         "}, {";
  s.resize(s.size() - 3);
  s += '}';
  return s;
}

std::string to_string(const Unit &unit) {
  switch (unit.id()) {
  case Unit::Id::Dimensionless:
    return "Unit::Dimensionless";
  case Unit::Id::Length:
    return "Unit::Length";
  default:
    return "<unknown unit>";
  }
}

std::string to_string(const Dataset &dataset) {
  std::string s("Dataset with ");
  s += std::to_string(dataset.size()) + " variables";
  return s;
}
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
void equals(const Unit &a, const Unit &b) {
  if (!(a == b))
    throw except::UnitMismatchError(a, b);
}
} // namespace expect
} // namespace dataset
