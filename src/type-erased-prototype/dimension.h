#ifndef DIMENSION_H
#define DIMENSION_H

// TODO rename to Spectrum and Detector?
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
  Row
};

#endif // DIMENSION_H
