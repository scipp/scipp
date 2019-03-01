/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef EXCEPT_H
#define EXCEPT_H

#include <stdexcept>
#include <string>

#include <gsl/gsl_util>

#include "dimension.h"
#include "unit.h"

class ConstDatasetSlice;
class Dataset;
class Dimensions;
class Tag;
class Unit;
class Variable;
class ConstVariableSlice;

namespace dataset {
std::string to_string(const Dim dim, const std::string &separator = "::");
std::string to_string(const Dimensions &dims,
                      const std::string &separator = "::");
std::string to_string(const Tag tag, const std::string &separator = "::");
std::string to_string(const Unit &unit, const std::string &separator = "::");
std::string to_string(const Variable &variable,
                      const std::string &separator = "::");
std::string to_string(const ConstVariableSlice &variable,
                      const std::string &separator = "::");
std::string to_string(const Dataset &dataset,
                      const std::string &separator = "::");
std::string to_string(const ConstDatasetSlice &dataset,
                      const std::string &separator = "::");

namespace except {

struct DimensionError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct DimensionMismatchError : public DimensionError {
  DimensionMismatchError(const Dimensions &expected, const Dimensions &actual);
};

struct DimensionNotFoundError : public DimensionError {
  DimensionNotFoundError(const Dimensions &expected, const Dim actual);
};

struct DimensionLengthError : public DimensionError {
  DimensionLengthError(const Dimensions &expected, const Dim actual,
                       const gsl::index length);
};

struct DatasetError : public std::runtime_error {
  DatasetError(const Dataset &dataset, const std::string &message);
  DatasetError(const ConstDatasetSlice &dataset, const std::string &message);
};

struct VariableNotFoundError : public DatasetError {
  VariableNotFoundError(const Dataset &dataset, const Tag tag,
                        const std::string &name);
  VariableNotFoundError(const ConstDatasetSlice &dataset, const Tag tag,
                        const std::string &name);
};

struct UnitError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct UnitMismatchError : public UnitError {
  UnitMismatchError(const Unit &a, const Unit &b);
};

} // namespace except

namespace expect {
void dimensionMatches(const Dimensions &dims, const Dim dim,
                      const gsl::index length);
void equals(const Unit &a, const Unit &b);

template <class T> void contains(const T &a, const T &b) {
  if (!a.contains(b))
    throw std::runtime_error("Expected " + to_string(a) + " to contain " +
                             to_string(b) + ".");
}
template <class T> void unit(const T &object, const Unit &unit) {
  expect::equals(object.unit(), unit);
}

template <class T> void countsOrCountsDensity(const T &object) {
  if (!(units::containsCounts(object.unit()) ||
        units::containsCountsVariance(object.unit())))
    throw except::UnitError("Expected counts or counts-density, got " +
                            object.unit().name() + '.');
}
} // namespace expect
} // namespace dataset

#endif // EXCEPT_H
