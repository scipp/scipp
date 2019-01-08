#ifndef EXCEPT_H
#define EXCEPT_H

#include <stdexcept>

#include <gsl/gsl_util>

#include "dimension.h"

class ConstDatasetSlice;
class Dataset;
class Dimensions;
class Tag;
class Unit;

namespace dataset {
std::string to_string(const Dim dim);
std::string to_string(const Dimensions &dims);
std::string to_string(const Unit &unit);

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
void equals(const Unit &a, const Unit &b);

template <class T> void contains(const T &a, const T &b) {
  if (!a.contains(b))
    throw std::runtime_error("Expected " + to_string(a) + " to contain " +
                             to_string(b) + ".");
}
} // namespace expect
} // namespace dataset

#endif // EXCEPT_H
