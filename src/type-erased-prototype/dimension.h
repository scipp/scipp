#ifndef DIMENSION_H
#define DIMENSION_H

// TODO rename to Spectrum and Detector?
enum class Dimension : char {
  Tof,
  Spectrum,
  SpectrumNumber,
  Run,
  Detector,
  Q,
  X,
  Y,
  Z,
  Row
};

#endif // DIMENSION_H
