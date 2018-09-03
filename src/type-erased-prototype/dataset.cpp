/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <set>

#include "dataset.h"

void Dataset::insert(Variable variable) {
  if (variable.isCoord() && count(variable.type()))
    throw std::runtime_error("Attempt to insert duplicate coordinate.");
  if (!variable.isCoord()) {
    for (const auto &item : m_variables)
      if (item.type() == variable.type() && item.name() == variable.name())
        throw std::runtime_error(
            "Attempt to insert data of same type with duplicate name.");
  }
  // TODO special handling for special variables types like
  // Data::Histogram (either prevent adding, or extract into underlying
  // variables).
  mergeDimensions(variable.dimensions());
  m_variables.push_back(std::move(variable));
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

void Dataset::insertAsEdge(const Dimension dimension, Variable variable) {
  // Edges are by 1 longer than other data, so dimension size check and
  // merging uses modified dimensions.
  auto dims = variable.dimensions();
  dims.resize(dimension, dims.size(dimension) - 1);
  mergeDimensions(dims);
  m_variables.push_back(std::move(variable));
}

gsl::index Dataset::find(const uint16_t id, const std::string &name) const {
  for (gsl::index i = 0; i < size(); ++i)
    if (m_variables[i].type() == id && m_variables[i].name() == name)
      return i;
  throw std::runtime_error("Dataset does not contain such a variable.");
}

gsl::index Dataset::count(const uint16_t id) const {
  gsl::index n = 0;
  for (auto &item : m_variables)
    if (item.type() == id)
      ++n;
  return n;
}

gsl::index Dataset::count(const uint16_t id, const std::string &name) const {
  gsl::index n = 0;
  for (auto &item : m_variables)
    if (item.type() == id && item.name() == name)
      ++n;
  return n;
}

gsl::index Dataset::findUnique(const uint16_t id) const {
  gsl::index index = -1;
  for (gsl::index i = 0; i < size(); ++i) {
    if (m_variables[i].type() == id) {
      if (index != -1)
        throw std::runtime_error(
            "Given variable tag is not unique. Must provide a name.");
      index = i;
    }
  }
  if (index == -1)
    throw std::runtime_error("Dataset does not contain such a variable.");
  return index;
}

void Dataset::mergeDimensions(const auto &dims) {
  gsl::index j = 0;
  gsl::index found = 0;
  for (gsl::index i = 0; i < dims.count(); ++i) {
    const auto dim = dims.label(i);
    const auto size = dims.size(i);
    bool found = false;
    for (; j < m_dimensions.count(); ++j) {
      if (m_dimensions.label(j) == dim) {
        if (m_dimensions.size(j) != size) // TODO compare ragged
          throw std::runtime_error(
              "Cannot insert variable into Dataset: Dimensions do not match");
        found = true;
        break;
      }
    }
    if (!found) {
      m_dimensions.add(dim, size);
    }
  }
}

Dataset &Dataset::operator+=(const Dataset &other) {
  for (const auto &var2 : other.m_variables) {
    // Handling of missing variables:
    // - Skip if this contains more (automatic by having enclosing loop over
    //   other instead of *this).
    // - Fail if other contains more.
    gsl::index index;
    try {
      index = find(var2.type(), var2.name());
    } catch (const std::runtime_error &) {
      throw std::runtime_error("Right-hand-side in addition contains variable "
                               "that is not present in left-hand-side.");
    }
    auto &var1 = m_variables[index];
    if (var1.isCoord()) {
      // Coordinate variables must match
      // Strictly speaking we should allow "equivalent" coordinates, i.e., match
      // only after projecting out any constant dimensions.
      if (!(var1 == var2))
        throw std::runtime_error(
            "Coordinates of datasets do not match. Cannot perform addition");
      // TODO We could improve sharing here magically, but whether this is
      // beneficial would depend on the shared reference count in var1 and var2:
      // var1 = var2;
    } else {
      // Data variables are added
      var1 += var2;
    }
  }
  return *this;
}

Dataset &Dataset::operator-=(const Dataset &other) {
  std::set<std::string> names;
  for (const auto &var2 : other.m_variables)
    if (!var2.isCoord())
      names.insert(var2.name());

  for (const auto &var2 : other.m_variables) {
    gsl::index index;
    try {
      index = find(var2.type(), var2.name());
    } catch (const std::runtime_error &) {
      if (!var2.isCoord() && names.size() == 1) {
        // Only a single (named) variable in RHS, subtract from all.
        index = -1;
      } else {
        throw std::runtime_error("Right-hand-side in subtraction contains "
                                 "variable that is not present in "
                                 "left-hand-side.");
      }
    }
    if (index >= 0) {
      auto &var1 = m_variables[index];
      if (var1.isCoord()) {
        if (!(var1 == var2))
          throw std::runtime_error(
              "Coordinates of datasets do not match. Cannot "
              "perform subtraction.");
      } else {
        var1 -= var2;
      }
    } else {
      // Not a coordinate, subtract from all.
      gsl::index count = 0;
      for (auto &var1 : m_variables) {
        if (var1.type() == var2.type()) {
          ++count;
          var1 -= var2;
          var1.setName(var1.name() + " - " + var2.name());
        }
      }
      if (count == 0)
        throw std::runtime_error("Right-hand-side in subtraction contains "
                                 "variable type that is not present in "
                                 "left-hand-side.");
    }
  }
  return *this;
}

namespace aligned {
// Helpers to define a pointer to aligned memory.
template <class T> using type alignas(32) = T;
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
}

Dataset &Dataset::operator*=(const Dataset &other) {
  // See operator+= for additional comments.
  for (const auto &var2 : other.m_variables) {
    gsl::index index;
    try {
      index = find(var2.type(), var2.name());
    } catch (const std::runtime_error &) {
      throw std::runtime_error("Right-hand-side in addition contains variable "
                               "that is not present in left-hand-side.");
    }
    if (var2.type() == tag_id<Data::Variance>) {
      try {
        find(tag_id<Data::Value>, var2.name());
        other.find(tag_id<Data::Value>, var2.name());
      } catch (const std::runtime_error &) {
        throw std::runtime_error("Cannot multiply datasets that contain a "
                                 "variance but no corresponding value.");
      }
    }
    auto &var1 = m_variables[index];
    if (var1.isCoord()) {
      // Coordinate variables must match
      if (!(var1 == var2))
        throw std::runtime_error(
            "Coordinates of datasets do not match. Cannot perform addition");
    } else {
      // Data variables are added
      if (var2.type() == tag_id<Data::Value>) {
        if (count(tag_id<Data::Variance>, var2.name()) !=
            other.count(tag_id<Data::Variance>, var2.name()))
          throw std::runtime_error("Either both or none of the operands must "
                                   "have a variance for their values.");
        if (count(tag_id<Data::Variance>, var2.name()) != 0) {
          auto error_index1 = find(tag_id<Data::Variance>, var2.name());
          auto error_index2 = other.find(tag_id<Data::Variance>, var2.name());
          auto &error1 = m_variables[error_index1];
          auto &error2 = other.m_variables[error_index2];
          if ((var1.dimensions() == var2.dimensions()) &&
              (var1.dimensions() == error1.dimensions()) &&
              (var1.dimensions() == error2.dimensions())) {
            // Optimization if all dimensions match, avoiding allocation of
            // temporaries and redundant streaming from memory of large array.
            error1.setUnit(var2.unit() * var2.unit() * error1.unit() +
                           var1.unit() * var1.unit() * error2.unit());
            var1.setUnit(var1.unit() * var2.unit());

            auto v1 = var1.get<Data::Value>();
            auto v2 = var2.get<const Data::Value>();
            auto e1 = error1.get<Data::Value>();
            auto e2 = error2.get<const Data::Value>();
            aligned::multiply(v1.size(), v1.data(), e1.data(), v2.data(),
                              e2.data());
          } else {
            error1 = error1 * (var2 * var2) + var1 * var1 * error2;
            // TODO: Catch errors from unit propagation here and give a better
            // error message.
            var1 *= var2;
          }
        } else {
          // No variance found, continue without.
          var1 *= var2;
        }
      } else if (var2.type() == tag_id<Data::Variance>) {
        // Do nothing, math for variance is done when processing corresponding
        // value.
      } else {
        var1 *= var2;
      }
    }
  }
  return *this;
}

void Dataset::setSlice(const Dataset &slice, const Dimension dim,
                       const gsl::index index) {
  for (const auto &var2 : slice.m_variables) {
    auto &var1 = m_variables[find(var2.type(), var2.name())];
    var1.setSlice(var2, dim, index);
  }
}

Dataset operator+(Dataset a, const Dataset &b) { return a += b; }
Dataset operator-(Dataset a, const Dataset &b) { return a -= b; }
Dataset operator*(Dataset a, const Dataset &b) { return a *= b; }

Dataset slice(const Dataset &d, const Dimension dim, const gsl::index index) {
  // TODO It is up for debate whether this should always throw if the dimension
  // is not contained or only with non-zero index.
  if (!d.dimensions().contains(dim) && index != 0)
    throw std::runtime_error("Slice index out of range");
  Dataset out;
  for (const auto &var : d) {
    if (var.dimensions().contains(dim))
      out.insert(slice(var, dim, index));
    else
      out.insert(var);
  }
  return out;
}

Dataset concatenate(const Dimension dim, const Dataset &d1, const Dataset &d2) {
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
    const auto &var2 = d2[d2.find(var1.type(), var1.name())];
    // TODO may need to extend things along constant dimensions to match shapes!
    if (var1.dimensions().contains(dim)) {
      out.insert(concatenate(dim, var1, var2));
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
          out.insert(concatenate(dim, var1, var2));
        }
      }
    }
  }
  return out;
}
