#ifndef EXCEPT_H
#define EXCEPT_H

#include <stdexcept>

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

namespace except {

struct DimensionError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct DimensionMismatchError : public DimensionError {
  DimensionMismatchError(const Dimensions &expected, const Dimensions &actual)
      : DimensionError("Expected dimensions " + to_string(expected) + ", got " +
                       to_string(actual) + "."),
        expected(expected), actual(actual) {}

  Dimensions expected;
  Dimensions actual;
};

struct DimensionNotFoundError : public DimensionError {
  DimensionNotFoundError(const Dimensions &expected, const Dim actual)
      : DimensionError("Expected dimension to be in " + to_string(expected) +
                       ", got " + to_string(actual) + "."),
        expected(expected), actual(actual) {}

  Dimensions expected;
  Dim actual;
};

struct DimensionLengthError : public DimensionError {
  DimensionLengthError(const Dimensions &expected, const Dim actual,
                       const gsl::index length)
      : DimensionError("Expected dimension to be in " + to_string(expected) +
                       ", got " + to_string(actual) +
                       " with mismatching length " + std::to_string(length) +
                       "."),
        expected(expected), actual(actual), length(length) {}

  Dimensions expected;
  Dim actual;
  gsl::index length;
};

} // namespace except
} // namespace dataset

#endif // EXCEPT_H
