/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DIMENSION_H
#define DIMENSION_H

enum class Dim : uint16_t {
  Invalid,
  Event,
  Tof,
  Spectrum,
  Monitor,
  Run,
  Detector,
  Q,
  X,
  Y,
  Z,
  Polarization,
  Temperature,
  Time,
  DetectorScan,
  Component,
  Row
};

constexpr bool isContinuous(const Dim dim) {
  if (dim == Dim::Tof || dim == Dim::X || dim == Dim::Y || dim == Dim::Z)
    return true;
  return false;
}

#endif // DIMENSION_H
