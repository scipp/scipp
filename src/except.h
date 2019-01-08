#ifndef EXCEPT_H
#define EXCEPT_H

#include <stdexcept>

#include <gsl/gsl_util>

#include "dimension.h"

class ConstDatasetSlice;
class Dataset;
class Dimensions;
class Tag;

namespace dataset {
std::string to_string(const Dim dim);
std::string to_string(const Dimensions &dims);

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

} // namespace except

namespace expect {
template <class T>
void contains(const T &a, const T &b,
              const std::string &context = std::string{},
              const std::string &solution = std::string{}) {
  if (!a.contains(b))
    throw std::runtime_error(std::string("While ") + context + ": " +
                             to_string(a) + " must contain " + to_string(b) +
                             ". " + solution);
}
} // namespace expect
} // namespace dataset

#endif // EXCEPT_H
