// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <functional>
#include <numeric>
#include <set>

#include "range/v3/algorithm.hpp"
#include "range/v3/view/zip.hpp"

#include "counts.h"
#include "dataset.h"
#include "except.h"
#include "tag_util.h"

namespace scipp::core {

Dataset::Dataset(std::vector<Item> items) {
  for (auto &item : items)
    insert(std::move(item));
}

Dataset::Dataset(const ConstDatasetSlice &view) {
  for (const auto & [ name, tag, data ] : view) {
    insert(tag, name, data);
  }
}

ConstDatasetSlice Dataset::subset(const std::string &name) const & {
  return ConstDatasetSlice(*this, name);
}

ConstDatasetSlice Dataset::subset(const Tag tag,
                                  const std::string &name) const & {
  return ConstDatasetSlice(*this, tag, name);
}

DatasetSlice Dataset::subset(const std::string &name) & {
  return DatasetSlice(*this, name);
}

DatasetSlice Dataset::subset(const Tag tag, const std::string &name) & {
  return DatasetSlice(*this, tag, name);
}

ConstDatasetSlice Dataset::operator()(const Dim dim, const scipp::index begin,
                                      const scipp::index end) const & {
  return ConstDatasetSlice(*this)(dim, begin, end);
}

Dataset Dataset::operator()(const Dim dim, const scipp::index begin,
                            const scipp::index end) && {
  return {DatasetSlice(*this)(dim, begin, end)};
}

DatasetSlice Dataset::operator()(const Dim dim, const scipp::index begin,
                                 const scipp::index end) & {
  return DatasetSlice(*this)(dim, begin, end);
}

ConstVariableSlice Dataset::operator()(const Tag tag,
                                       const std::string &name) const & {
  return ConstVariableSlice(std::get<Variable>(m_variables[find(tag, name)]));
}

VariableSlice Dataset::operator()(const Tag tag, const std::string &name) & {
  return VariableSlice(std::get<Variable>(m_variables[find(tag, name)]));
}

void Dataset::insert(Item item) {
  auto & [ name, tag, variable ] = item;
  // TODO special handling for special variables types like
  // Data::Histogram (either prevent adding, or extract into underlying
  // variables).
  if (contains(tag, name)) {
    auto &old = std::get<Variable>(m_variables[find(tag, name)]);
    for (const auto dim : old.dimensions().labels()) {
      bool found = false;
      for (const auto &var : m_variables) {
        if (std::get<Variable>(var) == old)
          continue;
        if (std::get<Variable>(var).dimensions().contains(dim))
          found = true;
      }
      if (!found)
        m_dimensions.erase(dim);
    }
    mergeDimensions(variable.dimensions(), coordDimension[tag.value()]);
    old = std::move(variable);
  } else {
    mergeDimensions(variable.dimensions(), coordDimension[tag.value()]);
    m_variables.emplace_back(name, tag, std::move(variable));
  }
}

// T can be Dataset or Slice.
template <class T>
bool contains(const T &dataset, const Tag tag, const std::string &name) {
  for (const auto & [ n, t, var ] : dataset)
    if (t == tag && n == name)
      return true;
  return false;
}

bool Dataset::contains(const Tag tag, const std::string &name) const {
  return scipp::core::contains(*this, tag, name);
}

Dataset::Item Dataset::erase(const Tag tag, const std::string &name) {
  const auto it = m_variables.begin() + find(tag, name);
  const auto dims = std::get<Variable>(*it).dimensions();
  Item item(std::move(*it));
  m_variables.erase(it);
  for (const auto dim : dims.labels()) {
    bool found = false;
    for (const auto &var : m_variables)
      if (std::get<Variable>(var).dimensions().contains(dim))
        found = true;
    if (!found)
      m_dimensions.erase(dim);
  }
  return item;
}

Dataset Dataset::extract(const std::string &name) {
  Dataset subset;
  for (auto it = m_variables.begin(); it != m_variables.end();) {
    if (std::get<std::string>(*it) == name) {
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
  for (const auto & [ name, tag, var ] : other) {
    if (contains(tag, name)) {
      if (var != operator()(tag, name))
        throw std::runtime_error("Cannot merge: Variable found in both "
                                 "operands, but does not match.");
    } else {
      insert(tag, name, var);
    }
  }
}

scipp::index Dataset::find(const Tag tag, const std::string &name) const {
  return scipp::core::find(*this, tag, name);
}

void Dataset::mergeDimensions(const Dimensions &dims, const Dim coordDim) {
  for (scipp::index i = 0; i < dims.count(); ++i) {
    const auto dim = dims.label(i);
    auto size = dims.size(i);
    bool found = false;
    for (scipp::index j = 0; j < m_dimensions.count(); ++j) {
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
              "coordinate, but the dimension length matches neither as default "
              "coordinate nor as edge coordinate.");
        } else {
          if (m_dimensions.size(j) == size + 1) {
            // If the dataset so far contains only edge variables for this
            // dimension, shrink its size.
            bool canShrink = true;
            for (const auto & [ name, tag, var ] : m_variables) {
              if (var.dimensions().contains(dim) &&
                  coordDimension[tag.value()] != dim)
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
  for (const auto & [ name, tag, var ] : a)
    if (!b.contains(tag, name) || (var != b(tag, name)))
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
  return std::get<ConstVariableSlice>(operator[](find(*this, tag, name)));
}

VariableSlice DatasetSlice::operator()(const Tag tag,
                                       const std::string &name) const {
  return std::get<VariableSlice>(operator[](find(*this, tag, name)));
}

/// Unified implementation for any in-place binary operation that requires
/// adding variances (+= and -=).
template <class Op, class T1, class T2>
T1 &binary_op_equals(Op op, T1 &dataset, const T2 &other) {
  std::set<std::string> names;
  for (const auto & [ name2, tag2, var2 ] : other)
    if (tag2.isData())
      names.insert(name2);

  for (const auto & [ name2, tag2, var2 ] : other) {
    // Handling of missing variables:
    // - Skip if this contains more (automatic by having enclosing loop over
    //   other instead of *this).
    // - Fail if other contains more.
    try {
      auto var1 = dataset(tag2, name2);
      if (tag2.isCoord()) {
        // Coordinate variables must match
        // Strictly speaking we should allow "equivalent" coordinates, i.e.,
        // match only after projecting out any constant dimensions.
        expect::variablesMatch(var1, var2);
        // TODO We could improve sharing here magically, but whether this is
        // beneficial would depend on the shared reference count in var1 and
        // var2: var1 = var2;
      } else if (tag2.isData()) {
        // Data variables are added
        if (tag2 == Data::Variance)
          var1 += var2;
        else
          op(var1, var2);
      } else {
        // Attribute variables are added
        // TODO Does it make sense to do this only if mismatched?
        if (var1 != var2)
          var1 += var2;
      }
    } catch (const except::VariableNotFoundError &) {
      // Note that this is handled via name, i.e., there may be values and
      // variances, i.e., two variables.
      if (tag2.isData() && names.size() == 1) {
        // Only a single (named) variable in RHS, subtract from all.
        // Not a coordinate, subtract from all.
        scipp::index count = 0;
        for (auto[name1, tag1, var1] : dataset) {
          if (tag1 == tag2) {
            ++count;
            if (tag1 == Data::Variance)
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
void multiply_data(const scipp::index size, ptr<double> RESTRICT v1,
                   ptr<double> RESTRICT e1, ptr<const double> RESTRICT v2,
                   ptr<const double> RESTRICT e2) {
  for (scipp::index i = 0; i < size; ++i) {
    e1[i] = e1[i] * (v2[i] * v2[i]) + e2[i] * (v1[i] * v1[i]);
    v1[i] *= v2[i];
  }
}
#define RESTRICT __restrict
void divide_data(const scipp::index size, ptr<double> RESTRICT v1,
                 ptr<double> RESTRICT e1, ptr<const double> RESTRICT v2,
                 ptr<const double> RESTRICT e2) {
  for (scipp::index i = 0; i < size; ++i) {
    e1[i] =
        (e1[i] + e2[i] * (v1[i] * v1[i]) / (v2[i] * v2[i])) / (v2[i] * v2[i]);
    v1[i] /= v2[i];
  }
}

void multiply(VariableSlice &lhs_var, const ConstVariableSlice &rhs_var,
              VariableSlice &lhs_err, const ConstVariableSlice &rhs_err) {
  auto v1 = lhs_var.template span<double>();
  const auto v2 = rhs_var.template span<double>();
  auto e1 = lhs_err.template span<double>();
  const auto e2 = rhs_err.template span<double>();
  multiply_data(v1.size(), v1.data(), e1.data(), v2.data(), e2.data());
  lhs_var.setUnit(lhs_var.unit() * rhs_var.unit());
  lhs_err.setUnit(lhs_var.unit() * lhs_var.unit());
}

void multiply(VariableSlice &lhs, const ConstVariableSlice &rhs) { lhs *= rhs; }

void divide(VariableSlice &lhs_var, const ConstVariableSlice &rhs_var,
            VariableSlice &lhs_err, const ConstVariableSlice &rhs_err) {
  auto v1 = lhs_var.template span<double>();
  const auto v2 = rhs_var.template span<double>();
  auto e1 = lhs_err.template span<double>();
  const auto e2 = rhs_err.template span<double>();
  divide_data(v1.size(), v1.data(), e1.data(), v2.data(), e2.data());
  lhs_var.setUnit(lhs_var.unit() / rhs_var.unit());
  lhs_err.setUnit(lhs_var.unit() * lhs_var.unit());
}

void divide(VariableSlice &lhs, const ConstVariableSlice &rhs) { lhs /= rhs; }

typedef void (*op_def)(VariableSlice &lhs, const ConstVariableSlice &rhs);
typedef void (*op_with_error_def)(VariableSlice &lhs_var,
                                  const ConstVariableSlice &rhs_var,
                                  VariableSlice &lhs_err,
                                  const ConstVariableSlice &rhs_err);
} // namespace aligned

namespace {

template <class T1, class T2>
void validate_operands(DatasetSlice &lhs_slice,
                       const ConstDatasetSlice &rhs_slice, const T1 &lhs_var,
                       const T2 &rhs_var) {
  const auto & [ lhs_name, lhs_tag, lhs_data ] = lhs_var;
  const auto & [ rhs_name, rhs_tag, rhs_data ] = rhs_var;
  if (lhs_slice.contains(Data::Variance, lhs_name) !=
      rhs_slice.contains(Data::Variance, rhs_name)) {
    throw std::runtime_error("Either both or none of the operands must "
                             "have a variance for their values.");
  }
  if (rhs_tag == Data::Variance) {
    if (!lhs_slice.contains(Data::Value, lhs_name) ||
        !rhs_slice.contains(Data::Value, rhs_name)) {
      throw std::runtime_error("Cannot operate on datasets that contain a "
                               "variance but no corresponding value.");
    }
  }
}

/**
 * @param lhs_slice : Slice of target data variable (lh operand), but should
 * have variances captured too if relvant
 * @param rhs_slice : Slice of target data variable (rh operand), but should
 * have variances captured too if relevant
 * @param lhs_var : lh variable (mutable)
 * @param rhs_var : rh variable (const)
 * @param op_with_error : inplace operator to use on operands. Handles value
 * and error at the data level.
 * @param op : inplace operator to use on operands of type
 * constvariableslice. No error considered.
 */
template <class T1, class T2>
void operate_on_slices(DatasetSlice &lhs_slice,
                       const ConstDatasetSlice &rhs_slice, const T1 &lhs_var,
                       const T2 &rhs_var,
                       aligned::op_with_error_def op_with_error,
                       aligned::op_def op) {
  auto[lhs_name, lhs_tag, lhs_data] = lhs_var;
  const auto & [ rhs_name, rhs_tag, rhs_data ] = rhs_var;
  if (rhs_tag == Data::Value) {
    if (lhs_slice.contains(Data::Variance, lhs_name)) {
      auto error1 = lhs_slice(Data::Variance, lhs_name);
      const auto &error2 = rhs_slice(Data::Variance, rhs_name);
      if ((lhs_data.dimensions() == rhs_data.dimensions()) &&
          (lhs_data.dimensions() == error1.dimensions()) &&
          (lhs_data.dimensions() == error2.dimensions())) {
        // Optimization if all dimensions match, avoiding allocation of
        // temporaries and redundant streaming from memory of large array.
        op_with_error(lhs_data, rhs_data, error1, error2);
      } else {
        // TODO Do we need to write this differently if the two operands
        // are the same? For example, error1 = error1 * (rhs_var * rhs_var) +
        // lhs_var * lhs_var * error2;
        // error message.
        const auto error2_shaped = broadcast(error2, lhs_data.dimensions());
        const auto rhs_var_shaped = broadcast(rhs_data, lhs_data.dimensions());
        ConstVariableSlice rhs_var_shaped_slice(rhs_var_shaped);
        ConstVariableSlice error2_shaped_slice(error2_shaped);
        op_with_error(lhs_data, rhs_var_shaped_slice, error1,
                      error2_shaped_slice);
      }
    } else {
      // No variance found, continue without.
      op(lhs_data, rhs_data);
    }
  } else if (rhs_tag == Data::Variance) {
    // Do nothing, math for variance is done when processing corresponding
    // value.
  } else {
    op(lhs_data, rhs_data);
  }
}

} // namespace
template <class T1, class T2>
T1 &op_equals(T1 &dataset, const T2 &other,
              aligned::op_with_error_def op_with_error, aligned::op_def op) {
  std::set<std::string> lhs_names;
  for (const auto &rhs_var : other)
    if (std::get<Tag>(rhs_var).isData())
      lhs_names.insert(std::get<const std::string &>(rhs_var));

  // See operator+= for additional comments.
  for (const auto &rhs_var : other) {
    // Look for exact match in lhs for var in rhs
    const auto & [ rhs_name, rhs_tag, rhs_data ] = rhs_var;
    if (dataset.contains(rhs_tag, rhs_name)) {
      auto lhs_var = dataset[find(dataset, rhs_tag, rhs_name)];
      const auto & [ lhs_name, lhs_tag, lhs_data ] = rhs_var;
      if (rhs_tag.isCoord()) {
        // Coordinate variables must match
        expect::variablesMatch(lhs_data, rhs_data);
      } else if (rhs_tag.isData()) {
        // Use slices to capture related variables for example variance data
        // variables
        auto lhs_slice = dataset.subset(rhs_name);
        auto rhs_slice = other.subset(rhs_name);
        validate_operands(lhs_slice, rhs_slice, lhs_var, rhs_var);
        if (rhs_tag != Data::Variance)
          operate_on_slices(lhs_slice, rhs_slice, lhs_var, rhs_var,
                            op_with_error, op);
      }
    } else {
      // Note that this is handled via name, i.e., there may be values and
      // variances, i.e., two variables.
      if (rhs_tag.isData() && lhs_names.size() == 1) {
        // Only a single (named) variable in RHS, operate on all.
        // Not a coordinate, apply from all.
        // op([a, b], [c]) = [op(a, c), op(b, c)] is legal
        auto rhs_slice = other.subset(rhs_name);

        scipp::index count = 0;
        for (auto lhs_var : dataset) {
          auto[lhs_name, lhs_tag, lhs_data] = lhs_var;
          if (lhs_tag == rhs_tag) {
            ++count;
            auto lhs_slice = dataset.subset(lhs_name);
            validate_operands(lhs_slice, rhs_slice, lhs_var, rhs_var);
            if (rhs_tag != Data::Variance)
              operate_on_slices(lhs_slice, rhs_slice, lhs_var, rhs_var,
                                op_with_error, op);
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
Dataset &Dataset::operator+=(const Variable &other) {
  for (auto & [ name, tag, var ] : m_variables)
    if (tag == Data::Value)
      var += other;
  return *this;
}
Dataset &Dataset::operator+=(const double value) {
  for (auto & [ name, tag, var ] : m_variables)
    if (tag == Data::Value)
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

Dataset &Dataset::operator-=(const Variable &other) {
  for (auto & [ name, tag, var ] : m_variables)
    if (tag == Data::Value)
      var -= other;
  return *this;
}
Dataset &Dataset::operator-=(const double value) {
  for (auto & [ name, tag, var ] : m_variables)
    if (tag == Data::Value)
      var -= value;
  return *this;
}

Dataset &Dataset::operator*=(const Dataset &other) {
  return op_equals(*this, other, &aligned::multiply, &aligned::multiply);
}
Dataset &Dataset::operator*=(const ConstDatasetSlice &other) {
  return op_equals(*this, other, &aligned::multiply, &aligned::multiply);
}
Dataset &Dataset::operator*=(const double value) {
  for (auto & [ name, tag, var ] : m_variables)
    if (tag == Data::Value)
      var *= value;
    else if (tag == Data::Variance)
      var *= value * value;
  return *this;
}

Dataset &Dataset::operator*=(const Variable &other) {
  for (auto & [ name, tag, var ] : m_variables)
    if (tag == Data::Value)
      var *= other;
    else if (tag == Data::Variance)
      var *= other * other;
  return *this;
}
Dataset &Dataset::operator/=(const Dataset &other) {
  return op_equals(*this, other, &aligned::divide, &aligned::divide);
}
Dataset &Dataset::operator/=(const ConstDatasetSlice &other) {
  return op_equals(*this, other, &aligned::divide, &aligned::divide);
}

Dataset &Dataset::operator/=(const Variable &other) {
  for (auto & [ name, tag, var ] : m_variables)
    if (tag == Data::Value)
      var /= other;
    else if (tag == Data::Variance)
      var /= other * other;
  return *this;
}
Dataset &Dataset::operator/=(const double value) {
  for (auto & [ name, tag, var ] : m_variables)
    if (tag == Data::Value)
      var /= value;
    else if (tag == Data::Variance)
      var /= value * value;
  return *this;
}

bool ConstDatasetSlice::contains(const Tag tag, const std::string &name) const {
  return scipp::core::contains(*this, tag, name);
}

template <class T1, class T2> T1 &assign(T1 &dataset, const T2 &other) {
  for (const auto & [ name2, tag2, var2 ] : other) {
    scipp::index index;
    try {
      index = find(dataset, tag2, name2);
    } catch (const std::runtime_error &) {
      throw std::runtime_error(
          "Right-hand-side in assignment contains variable "
          "that is not present in left-hand-side.");
    }
    auto[name1, tag1, var1] = dataset[index];
    if (tag1.isCoord()) {
      if (!(var1 == var2))
        throw std::runtime_error(
            "Coordinates of datasets do not match. Cannot assign.");
    } else if (tag1.isData()) {
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
  return scipp::core::assign(*this, other);
}
DatasetSlice DatasetSlice::assign(const ConstDatasetSlice &other) const {
  return scipp::core::assign(*this, other);
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
DatasetSlice DatasetSlice::operator+=(const Variable &other) const {
  for (const auto[name, tag, var] : *this)
    if (tag == Data::Value)
      var += other;
  return *this;
}
DatasetSlice DatasetSlice::operator+=(const double value) const {
  for (const auto[name, tag, var] : *this)
    if (tag == Data::Value)
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

DatasetSlice DatasetSlice::operator-=(const Variable &other) const {
  for (const auto[name, tag, var] : *this)
    if (tag == Data::Value)
      var -= other;
  return *this;
}
DatasetSlice DatasetSlice::operator-=(const double value) const {
  for (const auto[name, tag, var] : *this)
    if (tag == Data::Value)
      var -= value;
  return *this;
}

DatasetSlice DatasetSlice::operator*=(const Dataset &other) const {
  return op_equals(*this, other, &aligned::multiply, &aligned::multiply);
}
DatasetSlice DatasetSlice::operator*=(const ConstDatasetSlice &other) const {
  return op_equals(*this, other, &aligned::multiply, &aligned::multiply);
}
DatasetSlice DatasetSlice::operator*=(const Variable &other) const {
  for (const auto[name, tag, var] : *this)
    if (tag == Data::Value)
      var *= other;
    else if (tag == Data::Variance)
      var *= other * other;
  return *this;
}
DatasetSlice DatasetSlice::operator*=(const double value) const {
  for (const auto[name, tag, var] : *this)
    if (tag == Data::Value)
      var *= value;
    else if (tag == Data::Variance)
      var *= value * value;
  return *this;
}
DatasetSlice DatasetSlice::operator/=(const Dataset &other) const {
  return op_equals(*this, other, &aligned::divide, &aligned::divide);
}
DatasetSlice DatasetSlice::operator/=(const ConstDatasetSlice &other) const {
  return op_equals(*this, other, &aligned::divide, &aligned::divide);
}
DatasetSlice DatasetSlice::operator/=(const Variable &other) const {
  for (const auto[name, tag, var] : *this)
    if (tag == Data::Value)
      var /= other;
    else if (tag == Data::Variance)
      var /= other * other;
  return *this;
}
DatasetSlice DatasetSlice::operator/=(const double value) const {
  for (const auto[name, tag, var] : *this)
    if (tag == Data::Value)
      var /= value;
    else if (tag == Data::Variance)
      var /= value * value;
  return *this;
}

// Note: The std::move here is necessary because RVO does not work for variables
// that are function parameters.
Dataset operator+(Dataset a, const Dataset &b) { return std::move(a += b); }
Dataset operator-(Dataset a, const Dataset &b) { return std::move(a -= b); }
Dataset operator*(Dataset a, const Dataset &b) { return std::move(a *= b); }
Dataset operator/(Dataset a, const Dataset &b) { return std::move(a /= b); }
Dataset operator+(Dataset a, const ConstDatasetSlice &b) {
  return std::move(a += b);
}
Dataset operator-(Dataset a, const ConstDatasetSlice &b) {
  return std::move(a -= b);
}
Dataset operator*(Dataset a, const ConstDatasetSlice &b) {
  return std::move(a *= b);
}
Dataset operator/(Dataset a, const ConstDatasetSlice &b) {
  return std::move(a /= b);
}
Dataset operator+(Dataset a, const Variable &b) { return std::move(a += b); }
Dataset operator-(Dataset a, const Variable &b) { return std::move(a -= b); }
Dataset operator*(Dataset a, const Variable &b) { return std::move(a *= b); }
Dataset operator/(Dataset a, const Variable &b) { return std::move(a /= b); }
Dataset operator+(Dataset a, const double b) { return std::move(a += b); }
Dataset operator-(Dataset a, const double b) { return std::move(a -= b); }
Dataset operator*(Dataset a, const double b) { return std::move(a *= b); }
Dataset operator/(Dataset a, const double b) { return std::move(a /= b); }
Dataset operator+(const double a, Dataset b) { return std::move(b += a); }
Dataset operator-(const double a, Dataset b) { return -(b -= a); }
Dataset operator*(const double a, Dataset b) { return std::move(b *= a); }
std::vector<Dataset> split(const Dataset &d, const Dim dim,
                           const std::vector<scipp::index> &indices) {
  throw std::runtime_error("Not implemented.");
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
  for (scipp::index i1 = 0; i1 < d1.size(); ++i1) {
    const auto & [ name1, tag1, var1 ] = d1[i1];
    const auto &var2 = d2(tag1, name1);
    // TODO may need to extend things along constant dimensions to match shapes!
    if (var1.dimensions().contains(dim) || var1.sparseDim() == dim) {
      if (var1.sparseDim() == dim) {
        out.insert(tag1, name1, concatenate(var1, var2, dim));
      } else {
        const auto extent = d1.dimensions()[dim];
        if (var1.dimensions()[dim] == extent)
          out.insert(tag1, name1, concatenate(var1, var2, dim));
        else {
          // Variable contains bin edges, check matching first/last boundary,
          // do not duplicate joint boundary.
          const auto extent2 = var2.dimensions()[dim];
          if (extent2 == d2.dimensions()[dim])
            throw std::runtime_error(
                "Cannot concatenate: Second variable is not an edge variable.");
          if (var1(dim, extent) != var2(dim, 0))
            throw std::runtime_error(
                "Cannot concatenate: Last bin edge of first "
                "edge variable does not match first bin "
                "edge of second edge variable.");
          out.insert(tag1, name1,
                     concatenate(var1, var2(dim, 1, extent2), dim));
        }
      }
    } else {
      if (var1 == var2) {
        out.insert(tag1, name1, var1);
      } else {
        if (d1.dimensions().contains(dim)) {
          // Variable does not contain dimension but Dataset does, i.e.,
          // Variable is constant. We need to extend it before concatenating.
          auto dims1 = var1.dimensions();
          dims1.add(dim, d1.dimensions()[dim]);
          auto dims2 = var2.dimensions();
          if (d2.dimensions().contains(dim) && !var2.dimensions().contains(dim))
            dims2.add(dim, d2.dimensions()[dim]);
          out.insert(
              tag1, name1,
              concatenate(broadcast(var1, dims1), broadcast(var2, dims2), dim));
        } else {
          // Creating a new dimension
          out.insert(tag1, name1, concatenate(var1, var2, dim));
        }
      }
    }
  }
  return out;
}

Dataset rebin(const Dataset &d, const Variable &newCoord) {
  // Rebinned dimension must be the inner dimension of the coordinate.
  const auto dim = newCoord.dimensions().inner();
  const auto &newDims = newCoord.dimensions();
  if (!isContinuous(dim))
    throw std::runtime_error(
        "The provided rebin coordinate is not a continuous coordinate.");
  const auto &oldCoord = d(dimensionCoord(dim));
  const auto &oldDims = oldCoord.dimensions();
  const auto &datasetDims = d.dimensions();
  if (!oldDims.contains(dim))
    throw std::runtime_error("Existing coordinate to be rebined lacks the "
                             "dimension corresponding to the new coordinate.");
  if (oldDims[dim] != datasetDims[dim] + 1)
    throw std::runtime_error("Existing coordinate to be rebinned is not a bin "
                             "edge coordinate. Use `resample` instead of rebin "
                             "or convert to histogram data first.");
  for (scipp::index i = 0; i < newDims.ndim(); ++i) {
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
  for (const auto & [ name, tag, var ] : d) {
    if (!var.dimensions().contains(dim)) {
      out.insert(tag, name, var);
    } else if (tag == dimensionCoord(dim)) {
      out.insert(tag, name, newCoord);
    } else {
      out.insert(tag, name, rebin(var, oldCoord, newCoord));
    }
  }
  return out;
}

/*
Dataset histogram(const Variable &var, const Variable &coord,
                  const std::string &name) {
  // TODO Is there are more generic way to find "histogrammable" data, not
  // specific to (neutron) events? Something like Data::ValueVector, i.e., any
  // data variable that contains a vector of values at each point?
  const auto &events = var.span<Dataset>();
  // TODO This way of handling events (and their units) as nested Dataset feels
  // a bit unwieldy. Would it be a better option to store TOF (or any derived
  // values) as simple vectors in Data::Events? There would be a separate
  // Data::PulseTimes (and Data::EventWeights). This can then be of arbitrary
  // type, unit conversion is reflected in the unit of Data::Events. The
  // implementation of `histogram` would then also be simplified since we do not
  // need to distinguish between Data::Tof, etc. (which we are anyway not doing
  // currently).
  expect::equals(events[0](Data::Tof).unit(), coord.unit());

  // TODO Can we reuse some code for bin handling from MDZipView?
  const auto binDim = coord.dimensions().inner();
  const scipp::index nBin = coord.dimensions()[binDim] - 1;
  Dimensions dims = var.dimensions();
  // Note that the event list contains, e.g, time-of-flight values, but *not* as
  // a coordinate. Therefore, it should not depend on, e.g., Dim::Tof.
  if (dims.contains(binDim))
    throw std::runtime_error(
        "Data to histogram depends on histogram dimension.");
  for (const auto &dim : coord.dimensions().labels()) {
    if (dim != binDim) {
      expect::dimensionMatches(dims, dim, coord.dimensions()[dim]);
    }
  }

  dims.addInner(binDim, nBin);
  const scipp::index nextEdgeOffset = coord.dimensions().offset(binDim);

  Dataset hist;
  hist.insert(dimensionCoord(binDim), coord);
  auto countsVar = makeVariable<double>(dims);
  countsVar.setUnit(units::counts);

  // Counts has outer dimensions as input, with a new inner dimension given by
  // the binning dimensions. We iterate over all dimensions as a flat array.
  auto counts = countsVar.span<double>();
  scipp::index cur = 0;
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
    for (scipp::index bin = 0; bin < nBin; ++bin) {
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
  hist.insert(Data::Value, name, countsVar);
  hist.insert(Data::Variance, name, std::move(countsVar));
  hist(Data::Variance, name).setUnit(units::counts * units::counts);
  return hist;
}
*/

Dataset histogram(const Dataset &d, const Variable &coord) {
  Dataset hist;
  for (const auto & [ name, tag, var ] : d)
    if (tag == Data::Events)
      throw std::runtime_error("Disabled for refactor");
  // hist.merge(histogram(var, coord, name));
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
    std::vector<scipp::index> indices(axis.size());
    std::iota(indices.begin(), indices.end(), 0);
    auto view = ranges::view::zip(axis, indices);
    using ranges::sort;
    sort(view.begin(), view.end(), [](const auto &a, const auto &b) {
      return std::get<0>(a) < std::get<0>(b);
    });
    // Joint code for all tags, extract into function to reduce instantiated
    // code size?
    for (const auto & [ n, t, var ] : d) {
      if (!var.dimensions().contains(sortDim))
        sorted.insert(t, n, var);
      else if (t == tag && n == name)
        sorted.insert(t, n, axisVar);
      else
        sorted.insert(t, n, permute(var, sortDim, indices));
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
  for (auto[name, tag, var] : d)
    if (var.dimensions().contains(dim))
      filtered.insert(tag, name, filter(var, select));
    else
      filtered.insert(tag, name, var);
  return filtered;
}

Dataset sum(const Dataset &d, const Dim dim) {
  Dataset summed;
  for (auto[name, tag, var] : d) {
    if (var.dimensions().contains(dim)) {
      if (tag.isData())
        summed.insert(tag, name, sum(var, dim));
    } else {
      summed.insert(tag, name, var);
    }
  }
  return summed;
}

Dataset mean(const Dataset &d, const Dim dim) {
  // TODO This is a naive mean not taking into account the axis. Should this do
  // something smarter for unevenly spaced data?
  for (auto[name, tag, var] : d) {
    const Dim coordDim = coordDimension[tag.value()];
    if (coordDim != Dim::Invalid && coordDim != dim) {
      if (var.dimensions().contains(dim))
        throw std::runtime_error(
            std::string("Cannot compute mean along ") + to_string(dim).c_str() +
            ": Dimension coordinate for dimension " +
            to_string(coordDim).c_str() +
            " depends also on the dimension. Rebin to common axis first.");
    }
  }
  Dataset m;
  for (auto[name, tag, var] : d) {
    if (var.dimensions().contains(dim)) {
      if (tag.isData()) {
        if (tag == Data::Variance) {
          // Standard deviation of the mean has an extra 1/sqrt(N). Note that
          // this is not included by the stand-alone mean(Variable), since that
          // would be confusing.
          double scale =
              1.0 / std::sqrt(static_cast<double>(var.dimensions()[dim]));
          m.insert(tag, name,
                   mean(var, dim) * makeVariable<double>({}, {scale}));
        } else {
          m.insert(tag, name, mean(var, dim));
        }
      }
    } else {
      m.insert(tag, name, var);
    }
  }
  return m;
}

Dataset integrate(const Dataset &d, const Dim dim) {
  for (auto[name, tag, var] : d) {
    const Dim coordDim = coordDimension[tag.value()];
    if (coordDim != Dim::Invalid && coordDim != dim) {
      if (var.dimensions().contains(dim))
        throw std::runtime_error(
            std::string("Cannot compute mean along ") + to_string(dim).c_str() +
            ": Dimension coordinate for dimension " +
            to_string(coordDim).c_str() +
            " depends also on the dimension. Rebin to common axis first.");
    }
  }
  for (auto[name, tag, var] : d) {
    const Dim coordDim = coordDimension[tag.value()];
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

Dataset reverse(const Dataset &d, const Dim dim) {
  Dataset out;
  for (const auto[name, tag, var] : d)
    if (var.dimensions().contains(dim))
      out.insert(tag, name, reverse(var, dim));
    else
      out.insert(tag, name, var);
  return out;
}
} // namespace scipp::core
