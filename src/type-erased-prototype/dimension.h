/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DIMENSION_H
#define DIMENSION_H

enum class Dimension : char {
  Tof,
  MonitorTof,
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
  DetectorScan,
  Row
};

#endif // DIMENSION_H
