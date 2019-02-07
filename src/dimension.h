/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DIMENSION_H
#define DIMENSION_H

#include <cstdint>

enum class Dim : uint16_t {
  Component,
  DeltaE,
  Detector,
  DetectorScan,
  Energy,
  Event,
  Invalid,
  Monitor,
  Polarization,
  Position,
  Q,
  Row,
  Run,
  Spectrum,
  Temperature,
  Time,
  Tof,
  X,
  Y,
  Z
};

constexpr bool isContinuous(const Dim dim) {
  if (dim == Dim::Tof || dim == Dim::X || dim == Dim::Y || dim == Dim::Z)
    return true;
  return false;
}

#endif // DIMENSION_H
