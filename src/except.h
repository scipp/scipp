#ifndef EXCEPT_H
#define EXCEPT_H

#include <stdexcept>

#include <gsl/gsl_util>

#include "dimension.h"

class Dimensions;

namespace dataset {
std::string to_string(const Dim dim);
std::string to_string(const Dimensions &dims);

namespace except {

struct DimensionError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct DimensionMismatchError : public DimensionError {
  DimensionMismatchError(const Dimensions &expected, const Dimensions &actual)
      : DimensionError("Expected dimensions " + to_string(expected) + ", got " +
                       to_string(actual) + ".") {}
};

struct DimensionNotFoundError : public DimensionError {
  DimensionNotFoundError(const Dimensions &expected, const Dim actual)
      : DimensionError("Expected dimension to be in " + to_string(expected) +
                       ", got " + to_string(actual) + ".") {}
};

struct DimensionLengthError : public DimensionError {
  DimensionLengthError(const Dimensions &expected, const Dim actual,
                       const gsl::index length)
      : DimensionError("Expected dimension to be in " + to_string(expected) +
                       ", got " + to_string(actual) +
                       " with mismatching length " + std::to_string(length) +
                       ".") {}
};

} // namespace except
} // namespace dataset

#endif // EXCEPT_H
