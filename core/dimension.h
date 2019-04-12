// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DIMENSION_H
#define DIMENSION_H

#include <cstdint>

namespace scipp::core {

enum class Dim : uint16_t {
  Component,
  DeltaE,
  Detector,
  DetectorScan,
  DSpacing,
  Energy,
  Event,
  Invalid,
  Monitor,
  Polarization,
  Position,
  Q,
  Qx,
  Qy,
  Qz,
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
  if (dim == Dim::DSpacing || dim == Dim::Energy || dim == Dim::DeltaE ||
      dim == Dim::Tof || dim == Dim::X || dim == Dim::Y || dim == Dim::Z)
    return true;
  return false;
}

} // namespace scipp::core

#endif // DIMENSION_H
