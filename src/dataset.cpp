/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <numeric>
#include <set>

#include "range/v3/algorithm.hpp"
#include "range/v3/view/zip.hpp"

#include "counts.h"
#include "dataset.h"
#include "except.h"
#include "tag_util.h"

Dataset::Dataset(const ConstDatasetSlice &view) {
  for (const auto &var : view)
    insert(var);
}

ConstDatasetSlice Dataset::operator[](const std::string &name) const & {
  return ConstDatasetSlice(*this, name);
}

DatasetSlice Dataset::operator[](const std::string &name) & {
  return DatasetSlice(*this, name);
}

ConstDatasetSlice Dataset::operator()(const Dim dim, const gsl::index begin,
                                      const gsl::index end) const & {
  return ConstDatasetSlice(*this)(dim, begin, end);
}

Dataset Dataset::operator()(const Dim dim, const gsl::index begin,
                                 const gsl::index end) && {
  return {DatasetSlice(*this)(dim, begin, end)};
}

DatasetSlice Dataset::operator()(const Dim dim, const gsl::index begin,
                                 const gsl::index end) & {
  return DatasetSlice(*this)(dim, begin, end);
}

ConstVariableSlice Dataset::operator()(const Tag tag,
                                       const std::string &name) const & {
  return ConstVariableSlice(m_variables[find(tag, name)]);
}

VariableSlice Dataset::operator()(const Tag tag, const std::string &name) & {
  return VariableSlice(m_variables[find(tag, name)]);
}

void Dataset::insert(Variable variable) {
  if (variable.isCoord() && count(*this, variable.tag()))
    throw std::runtime_error("Attempt to insert duplicate coordinate.");
  if (!variable.isCoord()) {
    for (const auto &item : m_variables)
      if (item.tag() == variable.tag() && item.name() == variable.name())
        throw std::runtime_error(
            "Attempt to insert data with duplicate tag and name.");
  }
  // TODO special handling for special variables types like
  // Data::Histogram (either prevent adding, or extract into underlying
  // variables).
  mergeDimensions(variable.dimensions(),
                  coordDimension[variable.tag().value()]);
  m_variables.push_back(std::move(variable));
}

// T can be Dataset or Slice.
template <class T>
bool contains(const T &dataset, const Tag tag, const std::string &name) {
  for (gsl::index i = 0; i < dataset.size(); ++i)
    if (dataset[i].tag() == tag && dataset[i].name() == name)
      return true;
  return false;
}

bool Dataset::contains(const Tag tag, const std::string &name) const {
  return ::contains(*this, tag, name);
}

Variable Dataset::erase(const Tag tag, const std::string &name) {
  const auto it = m_variables.begin() + find(tag, name);
  const auto dims = it->dimensions();
  Variable var(std::move(*it));
  m_variables.erase(it);
  for (const auto dim : dims.labels()) {
    bool found = false;
    for (const auto &var : m_variables)
      if (var.dimensions().contains(dim))
        found = true;
    if (!found)
      m_dimensions.erase(dim);
  }
  return var;
}

Dataset Dataset::extract(const std::string &name) {
  Dataset subset;
  for (auto it = m_variables.begin(); it != m_variables.end();) {
    if (it->name() == name) {
      subset.insert(*it);
      it = m_variables.erase(it);
    } else {
      ++it;
    }
  }
  if (subset.size() == 0)
    throw std::runtime_error(
        "Dataset::extract(): No matching variable found in Dataset.");
  return subset;
}

void Dataset::merge(const Dataset &other) {
  for (const auto &var : other) {
    if (var.isCoord() && contains(var.tag(), var.name())) {
      if (var != operator()(var.tag(), var.name()))
        throw std::runtime_error("Cannot merge: Coordinates do not match.");
    } else {
      insert(var);
    }
  }
}

gsl::index Dataset::find(const Tag tag, const std::string &name) const {
  return ::find(*this, tag, name);
}

void Dataset::mergeDimensions(const Dimensions &dims, const Dim coordDim) {
  for (gsl::index i = 0; i < dims.count(); ++i) {
    const auto dim = dims.label(i);
    auto size = dims.size(i);
    bool found = false;
    for (gsl::index j = 0; j < m_dimensions.count(); ++j) {
      if (m_dimensions.label(j) == dim) {
        if (m_dimensions.size(j) == size) {
          found = true;
          break;
        }
        // coordDim is `Dim::Invalid` if there is no coordinate dimension.
        if (dim == coordDim) {
          if (m_dimensions.size(j) == size - 1) {
            // This is an edge coordinate, merge reduced dimension.
            --size;
            found = true;
            break;
          }
          throw std::runtime_error(
              "Cannot insert variable into Dataset: Variable is a dimension "
              "coordiante, but the dimension length matches neither as default "
              "coordinate nor as edge coordinate.");
        } else {
          if (m_dimensions.size(j) == size + 1) {
            // If the dataset so far contains only edge variables for this
            // dimension, shrink its size.
            bool canShrink = true;
            for (const auto &var : m_variables) {
              if (var.dimensions().contains(dim) &&
                  coordDimension[var.tag().value()] != dim)
                canShrink = false;
            }
            if (canShrink) {
              m_dimensions.resize(dim, size);
              found = true;
              break;
            }
          }
          throw std::runtime_error(
              "Cannot insert variable into Dataset: Dimensions do not match.");
        }
      }
    }
    // TODO Add after checking all so we can give strong exception guarantee.
    if (!found) {
      m_dimensions.add(dim, size);
    }
  }
}

template <class T1, class T2> bool equals(const T1 &a, const T2 &b) {
  if (a.size() != b.size())
    return false;
  for (const auto &var : a)
    if (!b.contains(var.tag(), var.name()) || (var != b(var.tag(), var.name())))
      return false;
  return true;
}

bool Dataset::operator==(const Dataset &other) const {
  return equals(*this, other);
}

bool Dataset::operator==(const ConstDatasetSlice &other) const {
  return equals(*this, other);
}

bool ConstDatasetSlice::operator==(const Dataset &other) const {
  return equals(*this, other);
}

bool ConstDatasetSlice::operator==(const ConstDatasetSlice &other) const {
  if ((m_dataset == other.m_dataset) && (m_indices == other.m_indices) &&
      (m_slices == other.m_slices))
    return true;
  return equals(*this, other);
}

bool Dataset::operator!=(const Dataset &other) const {
  return !operator==(other);
}

bool Dataset::operator!=(const ConstDatasetSlice &other) const {
  return !operator==(other);
}

bool ConstDatasetSlice::operator!=(const Dataset &other) const {
  return !operator==(other);
}

bool ConstDatasetSlice::operator!=(const ConstDatasetSlice &other) const {
  return !operator==(other);
}

ConstVariableSlice ConstDatasetSlice::
operator()(const Tag tag, const std::string &name) const {
  return ConstVariableSlice(operator[](find(*this, tag, name)));
}

VariableSlice DatasetSlice::operator()(const Tag tag,
                                       const std::string &name) const {
  return VariableSlice(operator[](find(*this, tag, name)));
}

/// Unified implementation for any in-place binary operation that requires
/// adding variances (+= and -=).
template <class Op, class T1, class T2>
T1 &binary_op_equals(Op op, T1 &dataset, const T2 &other) {
  std::set<std::string> names;
  for (const auto &var2 : other)
    if (var2.isData())
      names.insert(var2.name());

  for (const auto &var2 : other) {
    // Handling of missing variables:
    // - Skip if this contains more (automatic by having enclosing loop over
    //   other instead of *this).
    // - Fail if other contains more.
    try {
      auto var1 = dataset(var2.tag(), var2.name());
      if (var1.isCoord()) {
        // Coordinate variables must match
        // Strictly speaking we should allow "equivalent" coordinates, i.e.,
        // match only after projecting out any constant dimensions.
        dataset::expect::variablesMatch(var1, var2);
        // TODO We could improve sharing here magically, but whether this is
        // beneficial would depend on the shared reference count in var1 and
        // var2: var1 = var2;
      } else if (var1.isData()) {
        // Data variables are added
        if (var1.tag() == Data::Variance)
          var1 += var2;
        else
          op(var1, var2);
      } else {
        // Attribute variables are added
        // TODO Does it make sense to do this only if mismatched?
        if (var1 != var2)
          var1 += var2;
      }
    } catch (const dataset::except::VariableNotFoundError &) {
      // Note that this is handled via name, i.e., there may be values and
      // variances, i.e., two variables.
      if (var2.isData() && names.size() == 1) {
        // Only a single (named) variable in RHS, subtract from all.
        // Not a coordinate, subtract from all.
        gsl::index count = 0;
        for (auto var1 : dataset) {
          if (var1.tag() == var2.tag()) {
            ++count;
            if (var1.tag() == Data::Variance)
              var1 += var2;
            else
              op(var1, var2);
          }
        }
        if (count == 0)
          throw std::runtime_error("Right-hand-side in binary operation "
                                   "contains variable type that is not present "
                                   "in left-hand-side.");
      } else {
        throw std::runtime_error("Right-hand-side in binary operation contains "
                                 "variable that is not present in "
                                 "left-hand-side.");
      }
    }
  }
  return dataset;
}

namespace aligned {
// Helpers to define a pointer to aligned memory.
// alignas cannot be used like this, e.g., clang rejects it. Need to find
// another way.
// template <class T> using type alignas(32) = T;
template <class T> using type = T;
template <class T> using ptr = type<T> *;

// Using restrict does not seem to help much?
#define RESTRICT __restrict
void multiply(const gsl::index size, ptr<double> RESTRICT v1,
              ptr<double> RESTRICT e1, ptr<const double> RESTRICT v2,
              ptr<const double> RESTRICT e2) {
  for (gsl::index i = 0; i < size; ++i) {
    e1[i] = e1[i] * (v2[i] * v2[i]) + e2[i] * (v1[i] * v1[i]);
    v1[i] *= v2[i];
  }
}
} // namespace aligned

template <class T1, class T2> T1 &times_equals(T1 &dataset, const T2 &other) {
  // See operator+= for additional comments.
  for (const auto &var2 : other) {
    gsl::index index;
    try {
      index = find(dataset, var2.tag(), var2.name());
    } catch (const std::runtime_error &) {
      throw std::runtime_error("Right-hand-side in addition contains variable "
                               "that is not present in left-hand-side.");
    }
    if (var2.tag() == Data::Variance) {
      try {
        find(dataset, Data::Value, var2.name());
        find(other, Data::Value, var2.name());
      } catch (const std::runtime_error &) {
        throw std::runtime_error("Cannot multiply datasets that contain a "
                                 "variance but no corresponding value.");
      }
    }
    auto var1 = dataset[index];
    if (var1.isCoord()) {
      // Coordinate variables must match
      dataset::expect::variablesMatch(var1, var2);
    } else if (var1.isData()) {
      // Data variables are added
      if (var2.tag() == Data::Value) {
        if (count(dataset, Data::Variance, var2.name()) !=
            count(other, Data::Variance, var2.name()))
          throw std::runtime_error("Either both or none of the operands must "
                                   "have a variance for their values.");
        if (count(dataset, Data::Variance, var2.name()) != 0) {
          auto error_index1 = find(dataset, Data::Variance, var2.name());
          auto error_index2 = find(other, Data::Variance, var2.name());
          auto error1 = dataset[error_index1];
          const auto &error2 = other[error_index2];
          if ((var1.dimensions() == var2.dimensions()) &&
              (var1.dimensions() == error1.dimensions()) &&
              (var1.dimensions() == error2.dimensions())) {
            // Optimization if all dimensions match, avoiding allocation of
            // temporaries and redundant streaming from memory of large array.
            error1.setUnit(var2.unit() * var2.unit() * error1.unit() +
                           var1.unit() * var1.unit() * error2.unit());
            var1.setUnit(var1.unit() * var2.unit());

            // TODO We are working with VariableSlice here, so get<> returns a
            // view, not a span, i.e., it is less efficient. May need to do this
            // differently for optimal performance.
            auto v1 = var1.template span<double>();
            const auto v2 = var2.template span<double>();
            auto e1 = error1.template span<double>();
            const auto e2 = error2.template span<double>();
            // TODO Need to ensure that data is contiguous!
            aligned::multiply(v1.size(), v1.data(), e1.data(), v2.data(),
                              e2.data());
          } else {
            // TODO Do we need to write this differently if the two operands are
            // the same? For example,
            // error1 = error1 * (var2 * var2) + var1 * var1 * error2;
            error1 *= (var2 * var2);
            error1 += var1 * var1 * error2;
            // TODO: Catch errors from unit propagation here and give a better
            // error message.
            var1 *= var2;
          }
        } else {
          // No variance found, continue without.
          var1 *= var2;
        }
      } else if (var2.tag() == Data::Variance) {
        // Do nothing, math for variance is done when processing corresponding
        // value.
      } else {
        var1 *= var2;
      }
    }
  }
  return dataset;
}

Dataset Dataset::operator-() const {
  auto copy(*this);
  copy *= -1.0;
  return copy;
}

Dataset &Dataset::operator+=(const Dataset &other) {
  return binary_op_equals(
      [](VariableSlice &a, const ConstVariableSlice &b) { return a += b; },
      *this, other);
}
Dataset &Dataset::operator+=(const ConstDatasetSlice &other) {
  return binary_op_equals(
      [](VariableSlice &a, const ConstVariableSlice &b) { return a += b; },
      *this, other);
}
Dataset &Dataset::operator+=(const double value) {
  for (auto &var : m_variables)
    if (var.tag() == Data::Value)
      var += value;
  return *this;
}

Dataset &Dataset::operator-=(const Dataset &other) {
  return binary_op_equals(
      [](VariableSlice &a, const ConstVariableSlice &b) { return a -= b; },
      *this, other);
}
Dataset &Dataset::operator-=(const ConstDatasetSlice &other) {
  return binary_op_equals(
      [](VariableSlice &a, const ConstVariableSlice &b) { return a -= b; },
      *this, other);
}
Dataset &Dataset::operator-=(const double value) {
  for (auto &var : m_variables)
    if (var.tag() == Data::Value)
      var -= value;
  return *this;
}

Dataset &Dataset::operator*=(const Dataset &other) {
  return times_equals(*this, other);
}
Dataset &Dataset::operator*=(const ConstDatasetSlice &other) {
  return times_equals(*this, other);
}
Dataset &Dataset::operator*=(const double value) {
  for (auto &var : m_variables)
    if (var.tag() == Data::Value)
      var *= value;
    else if (var.tag() == Data::Variance)
      var *= value * value;
  return *this;
}

bool ConstDatasetSlice::contains(const Tag tag, const std::string &name) const {
  return ::contains(*this, tag, name);
}

template <class T1, class T2> T1 &assign(T1 &dataset, const T2 &other) {
  for (const auto &var2 : other) {
    gsl::index index;
    try {
      index = find(dataset, var2.tag(), var2.name());
    } catch (const std::runtime_error &) {
      throw std::runtime_error(
          "Right-hand-side in assignment contains variable "
          "that is not present in left-hand-side.");
    }
    auto var1 = dataset[index];
    if (var1.isCoord()) {
      if (!(var1 == var2))
        throw std::runtime_error(
            "Coordinates of datasets do not match. Cannot assign.");
    } else if (var1.isData()) {
      // Data variables are assigned
      var1.assign(var2);
    } else {
      // Attribute variables are assigned
      if (var1 != var2)
        var1 += var2;
    }
  }
  return dataset;
}

Dataset ConstDatasetSlice::operator-() const {
  Dataset copy(*this);
  return -copy;
}

DatasetSlice DatasetSlice::assign(const Dataset &other) const {
  return ::assign(*this, other);
}
DatasetSlice DatasetSlice::assign(const ConstDatasetSlice &other) const {
  return ::assign(*this, other);
}

DatasetSlice DatasetSlice::operator+=(const Dataset &other) const {
  return binary_op_equals(
      [](VariableSlice &a, const Variable &b) { return a += b; }, *this, other);
}
DatasetSlice DatasetSlice::operator+=(const ConstDatasetSlice &other) const {
  return binary_op_equals(
      [](VariableSlice &a, const ConstVariableSlice &b) { return a += b; },
      *this, other);
}
DatasetSlice DatasetSlice::operator+=(const double value) const {
  for (auto var : *this)
    if (var.tag() == Data::Value)
      var += value;
  return *this;
}

DatasetSlice DatasetSlice::operator-=(const Dataset &other) const {
  return binary_op_equals(
      [](VariableSlice &a, const Variable &b) { return a -= b; }, *this, other);
}
DatasetSlice DatasetSlice::operator-=(const ConstDatasetSlice &other) const {
  return binary_op_equals(
      [](VariableSlice &a, const ConstVariableSlice &b) { return a -= b; },
      *this, other);
}
DatasetSlice DatasetSlice::operator-=(const double value) const {
  for (auto var : *this)
    if (var.tag() == Data::Value)
      var -= value;
  return *this;
}

DatasetSlice DatasetSlice::operator*=(const Dataset &other) const {
  return times_equals(*this, other);
}
DatasetSlice DatasetSlice::operator*=(const ConstDatasetSlice &other) const {
  return times_equals(*this, other);
}
DatasetSlice DatasetSlice::operator*=(const double value) const {
  for (auto var : *this)
    if (var.tag() == Data::Value)
      var *= value;
    else if (var.tag() == Data::Variance)
      var *= value * value;
  return *this;
}

// Note: The std::move here is necessary because RVO does not work for variables
// that are function parameters.
Dataset operator+(Dataset a, const Dataset &b) { return std::move(a += b); }
Dataset operator-(Dataset a, const Dataset &b) { return std::move(a -= b); }
Dataset operator*(Dataset a, const Dataset &b) { return std::move(a *= b); }
Dataset operator+(Dataset a, const ConstDatasetSlice &b) {
  return std::move(a += b);
}
Dataset operator-(Dataset a, const ConstDatasetSlice &b) {
  return std::move(a -= b);
}
Dataset operator*(Dataset a, const ConstDatasetSlice &b) {
  return std::move(a *= b);
}
Dataset operator+(Dataset a, const double b) { return std::move(a += b); }
Dataset operator-(Dataset a, const double b) { return std::move(a -= b); }
Dataset operator*(Dataset a, const double b) { return std::move(a *= b); }
Dataset operator+(const double a, Dataset b) { return std::move(b += a); }
Dataset operator-(const double a, Dataset b) { return -(b -= a); }
Dataset operator*(const double a, Dataset b) { return std::move(b *= a); }

std::vector<Dataset> split(const Dataset &d, const Dim dim,
                           const std::vector<gsl::index> &indices) {
  std::vector<Dataset> out(indices.size() + 1);
  for (const auto &var : d) {
    if (var.dimensions().contains(dim)) {
      auto vars = split(var, dim, indices);
      for (size_t i = 0; i < out.size(); ++i)
        out[i].insert(vars[i]);
    } else {
      for (auto &o : out)
        o.insert(var);
    }
  }
  return out;
}

Dataset concatenate(const Dataset &d1, const Dataset &d2, const Dim dim) {
  // Match type and name, drop missing?
  // What do we have to do to check and compute the resulting dimensions?
  // - If dim is in m_dimensions, *some* of the variables contain it. Those that
  //   do not must then be identical (do not concatenate) or we could
  //   automatically broadcast? Yes!?
  // - If dim is new, concatenate variables if different, copy if same.
  // We will be doing deep comparisons here, it would be nice if we could setup
  // sharing, but d1 and d2 are const, is there a way...? Not without breaking
  // thread safety? Could cache cow_ptr for future sharing setup, done by next
  // non-const op?
  Dataset out;
  for (gsl::index i1 = 0; i1 < d1.size(); ++i1) {
    const auto &var1 = d1[i1];
    const auto &var2 = d2(var1.tag(), var1.name());
    // TODO may need to extend things along constant dimensions to match shapes!
    if (var1.dimensions().contains(dim)) {
      const auto extent = d1.dimensions()[dim];
      if (var1.dimensions()[dim] == extent)
        out.insert(concatenate(var1, var2, dim));
      else {
        // Variable contains bin edges, check matching first/last boundary,
        // do not duplicate joint boundary.
        const auto extent2 = var2.dimensions()[dim];
        if (extent2 == d2.dimensions()[dim])
          throw std::runtime_error(
              "Cannot concatenate: Second variable is not an edge variable.");
        if (var1(dim, extent) != var2(dim, 0))
          throw std::runtime_error("Cannot concatenate: Last bin edge of first "
                                   "edge variable does not match first bin "
                                   "edge of second edge variable.");
        out.insert(concatenate(var1, var2(dim, 1, extent2), dim));
      }
    } else {
      if (var1 == var2) {
        out.insert(var1);
      } else {
        if (d1.dimensions().contains(dim)) {
          // Variable does not contain dimension but Dataset does, i.e.,
          // Variable is constant. We need to extend it before concatenating.
          throw std::runtime_error("TODO");
        } else {
          // Creating a new dimension
          out.insert(concatenate(var1, var2, dim));
        }
      }
    }
  }
  return out;
}

Dataset rebin(const Dataset &d, const Variable &newCoord) {
  if (!newCoord.isCoord())
    throw std::runtime_error(
        "The provided rebin coordinate is not a coordinate variable.");
  const auto dim = coordDimension[newCoord.tag().value()];
  if (dim == Dim::Invalid)
    throw std::runtime_error(
        "The provided rebin coordinate is not a dimension coordinate.");
  const auto &newDims = newCoord.dimensions();
  if (!newDims.contains(dim))
    throw std::runtime_error("The provided rebin coordinate lacks the "
                             "dimension corresponding to the coordinate.");
  if (!isContinuous(dim))
    throw std::runtime_error(
        "The provided rebin coordinate is not a continuous coordinate.");
  const auto &oldCoord = d(Tag(newCoord.tag().value()));
  const auto &oldDims = oldCoord.dimensions();
  const auto &datasetDims = d.dimensions();
  if (!oldDims.contains(dim))
    throw std::runtime_error("Existing coordinate to be rebined lacks the "
                             "dimension corresponding to the new coordinate.");
  if (oldDims[dim] != datasetDims[dim] + 1)
    throw std::runtime_error("Existing coordinate to be rebinned is not a bin "
                             "edge coordinate. Use `resample` instead of rebin "
                             "or convert to histogram data first.");
  for (gsl::index i = 0; i < newDims.ndim(); ++i) {
    const auto newDim = newDims.label(i);
    if (newDim == dim)
      continue;
    if (datasetDims.contains(newDim)) {
      if (datasetDims[newDim] != newDims.shape()[i])
        throw std::runtime_error(
            "Size mismatch in auxiliary dimension of new coordinate.");
    }
  }
  // TODO check that input as well as output coordinate are sorted in rebin
  // dimension.
  Dataset out;
  for (const auto &var : d) {
    if (!var.dimensions().contains(dim)) {
      out.insert(var);
    } else if (var.tag() == newCoord.tag()) {
      out.insert(newCoord);
    } else {
      out.insert(rebin(var, oldCoord, newCoord));
    }
  }
  return out;
}

Dataset histogram(const Variable &var, const Variable &coord) {
  // TODO Is there are more generic way to find "histogrammable" data, not
  // specific to (neutron) events? Something like Data::ValueVector, i.e., any
  // data variable that contains a vector of values at each point?
  const auto &events = var.get(Data::Events);
  // TODO This way of handling events (and their units) as nested Dataset feels
  // a bit unwieldy. Would it be a better option to store TOF (or any derived
  // values) as simple vectors in Data::Events? There would be a separate
  // Data::PulseTimes (and Data::EventWeights). This can then be of arbitrary
  // type, unit conversion is reflected in the unit of Data::Events. The
  // implementation of `histogram` would then also be simplified since we do not
  // need to distinguish between Data::Tof, etc. (which we are anyway not doing
  // currently).
  dataset::expect::equals(events[0](Data::Tof).unit(), coord.unit());

  // TODO Can we reuse some code for bin handling from MDZipView?
  const auto binDim = coordDimension[coord.tag().value()];
  const gsl::index nBin = coord.dimensions()[binDim] - 1;
  Dimensions dims = var.dimensions();
  // Note that the event list contains, e.g, time-of-flight values, but *not* as
  // a coordinate. Therefore, it should not depend on, e.g., Dim::Tof.
  if (dims.contains(binDim))
    throw std::runtime_error(
        "Data to histogram depends on histogram dimension.");
  for (const auto &dim : coord.dimensions().labels()) {
    if (dim != binDim) {
      dataset::expect::dimensionMatches(dims, dim, coord.dimensions()[dim]);
    }
  }

  dims.addInner(binDim, nBin);
  const gsl::index nextEdgeOffset = coord.dimensions().offset(binDim);

  Dataset hist;
  hist.insert(coord);
  Variable countsVar(Data::Value, dims);
  countsVar.setUnit(units::counts);

  // Counts has outer dimensions as input, with a new inner dimension given by
  // the binning dimensions. We iterate over all dimensions as a flat array.
  auto counts = countsVar.get(Data::Value);
  gsl::index cur = 0;
  // The helper `getView` allows us to ignore the tag of coord, as long as the
  // underlying type is `double`. We view the edges with the same dimensions as
  // the output. This abstracts the differences between either a shared binning
  // axis or a potentially different binning for each event list.
  // TODO Need to add a branch for the `float` case.
  const auto edges = getView<double>(coord, dims);
  auto edge = edges.begin();
  for (const auto &eventList : events) {
    const auto tofs = eventList.get(Data::Tof);
    if (!std::is_sorted(tofs.begin(), tofs.end()))
      throw std::runtime_error(
          "TODO: Histograms can currently only be created from sorted data.");
    auto left = *edge;
    auto begin = std::lower_bound(tofs.begin(), tofs.end(), left);
    for (gsl::index bin = 0; bin < nBin; ++bin) {
      // The iterator cannot see the last edge, we must add the offset to the
      // memory location, *not* to the iterator.
      const auto right = *(&*edge + nextEdgeOffset);
      if (right < left)
        throw std::runtime_error(
            "Coordinate used for binning is not increasing.");
      const auto end = std::upper_bound(begin, tofs.end(), right);
      counts[cur] = std::distance(begin, end);
      begin = end;
      left = right;
      ++edge;
      ++cur;
    }
  }

  // TODO Would need to add handling for weighted events etc. here.
  hist.insert(Data::Value, var.name(), countsVar);
  hist.insert(Data::Variance, var.name(), std::move(countsVar));
  hist(Data::Variance, var.name()).setUnit(units::counts * units::counts);
  return hist;
}

Dataset histogram(const Dataset &d, const Variable &coord) {
  Dataset hist;
  for (const auto &var : d)
    if (var.tag() == Data::Events)
      hist.merge(histogram(var, coord));
  if (hist.size() == 0)
    throw std::runtime_error("Dataset does not contain any variables with "
                             "event data, cannot histogram.");
  return hist;
}

// We can specialize this to switch to a more efficient variant when sorting
// datasets that represent events lists, using ZipView.
template <class T> struct Sort {
  static Dataset apply(const Dataset &d, const Tag tag,
                       const std::string &name) {
    auto const_axis = d.span<T>(tag, name);
    if (d(tag, name).dimensions().count() != 1)
      throw std::runtime_error("Axis for sorting must be 1-dimensional.");
    const auto sortDim = d(tag, name).dimensions().label(0);
    if (const_axis.size() != d.dimensions()[sortDim])
      throw std::runtime_error("Axis for sorting cannot be a bin-edge axis.");
    if (std::is_sorted(const_axis.begin(), const_axis.end()))
      return d;

    Dataset sorted;
    Variable axisVar = d(tag, name);
    auto axis = axisVar.span<T>();
    std::vector<gsl::index> indices(axis.size());
    std::iota(indices.begin(), indices.end(), 0);
    auto view = ranges::view::zip(axis, indices);
    using ranges::sort;
    sort(view.begin(), view.end(), [](const auto &a, const auto &b) {
      return std::get<0>(a) < std::get<0>(b);
    });
    // Joint code for all tags, extract into function to reduce instantiated
    // code size?
    for (const auto &var : d) {
      if (!var.dimensions().contains(sortDim))
        sorted.insert(var);
      else if (var.tag() == tag && var.name() == name)
        sorted.insert(axisVar);
      else
        sorted.insert(permute(var, sortDim, indices));
    }
    return sorted;
  }
};

Dataset sort(const Dataset &d, const Tag t, const std::string &name) {
  return CallDType<double, float, int64_t, int32_t, std::string>::apply<Sort>(
      d(t, name).dtype(), d, t, name);
}

Dataset filter(const Dataset &d, const Variable &select) {
  if (select.dimensions().ndim() != 1)
    throw std::runtime_error(
        "Cannot filter variable: The filter must by 1-dimensional.");
  const auto dim = select.dimensions().labels()[0];

  Dataset filtered;
  for (auto var : d)
    if (var.dimensions().contains(dim))
      filtered.insert(filter(var, select));
    else
      filtered.insert(var);
  return filtered;
}

Dataset sum(const Dataset &d, const Dim dim) {
  Dataset summed;
  for (auto var : d) {
    if (var.dimensions().contains(dim)) {
      if (var.isData())
        summed.insert(sum(var, dim));
    } else {
      summed.insert(var);
    }
  }
  return summed;
}

Dataset mean(const Dataset &d, const Dim dim) {
  // TODO This is a naive mean not taking into account the axis. Should this do
  // something smarter for unevenly spaced data?
  for (auto var : d) {
    const Dim coordDim = coordDimension[var.tag().value()];
    if (coordDim != Dim::Invalid && coordDim != dim) {
      if (var.dimensions().contains(dim))
        throw std::runtime_error(
            std::string("Cannot compute mean along ") +
            dataset::to_string(dim).c_str() +
            ": Dimension coordinate for dimension " +
            dataset::to_string(coordDim).c_str() +
            " depends also on the dimension. Rebin to common axis first.");
    }
  }
  Dataset m;
  for (auto var : d) {
    if (var.dimensions().contains(dim)) {
      if (var.isData()) {
        if (var.tag() == Data::Variance) {
          // Standard deviation of the mean has an extra 1/sqrt(N). Note that
          // this is not included by the stand-alone mean(Variable), since that
          // would be confusing.
          double scale = 1.0 / sqrt(static_cast<double>(var.dimensions()[dim]));
          m.insert(mean(var, dim) * Variable(Data::Value, {}, {scale}));
        } else {
          m.insert(mean(var, dim));
        }
      }
    } else {
      m.insert(var);
    }
  }
  return m;
}

Dataset integrate(const Dataset &d, const Dim dim) {
  for (auto var : d) {
    const Dim coordDim = coordDimension[var.tag().value()];
    if (coordDim != Dim::Invalid && coordDim != dim) {
      if (var.dimensions().contains(dim))
        throw std::runtime_error(
            std::string("Cannot compute mean along ") +
            dataset::to_string(dim).c_str() +
            ": Dimension coordinate for dimension " +
            dataset::to_string(coordDim).c_str() +
            " depends also on the dimension. Rebin to common axis first.");
    }
  }
  for (auto var : d) {
    const Dim coordDim = coordDimension[var.tag().value()];
    if (coordDim == dim) {
      const auto size = var.dimensions()[dim];
      if (size != d.dimensions()[dim] + 1)
        throw std::runtime_error("Cannot integrate: Implemented only for "
                                 "histogram data (requires bin-edge "
                                 "coordinate.");
      const auto range = concatenate(var(dim, 0), var(dim, size - 1), dim);
      // TODO Currently this works only for counts and counts-density.
      const auto integral = rebin(d, range);
      // Return slice to automatically drop `dim` and corresponding coordinate.
      return counts::fromDensity(std::move(integral), dim)(dim, 0);
    }
  }
  throw std::runtime_error(
      "Integration required bin-edge dimension coordinate.");
}
