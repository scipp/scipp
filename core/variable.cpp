// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>
#include <string>

#include "operators.h"
#include "scipp/core/apply.h"
#include "scipp/core/counts.h"
#include "scipp/core/dtype.h"
#include "scipp/core/except.h"
#include "scipp/core/tag_util.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"
#include "scipp/core/variable.tcc"
#include "scipp/core/variable_view.h"

namespace scipp::core {

VariableConcept::VariableConcept(const Dimensions &dimensions)
    : m_dimensions(dimensions) {}

Variable::Variable(const VariableConstProxy &slice)
    : Variable(*slice.m_variable) {
  if (slice.m_view) {
    setUnit(slice.unit());
    setDims(slice.dims());
    // There is a bug in the implementation of MultiIndex used in VariableView
    // in case one of the dimensions has extent 0.
    if (dims().volume() != 0)
      data().copy(slice.data(), Dim::Invalid, 0, 0, 1);
  }
}

Variable::Variable(const Variable &parent, const Dimensions &dims)
    : m_unit(parent.unit()),
      m_object(parent.m_object->makeDefaultFromParent(dims)) {}

Variable::Variable(const VariableConstProxy &parent, const Dimensions &dims)
    : m_unit(parent.unit()),
      m_object(parent.data().makeDefaultFromParent(dims)) {}

Variable::Variable(const Variable &parent, VariableConceptHandle data)
    : m_unit(parent.unit()), m_object(std::move(data)) {}

void Variable::setDims(const Dimensions &dimensions) {
  if (dimensions.volume() == m_object->dims().volume()) {
    if (dimensions != m_object->dims())
      data().m_dimensions = dimensions;
    return;
  }
  m_object = m_object->makeDefaultFromParent(dimensions);
}
INSTANTIATE_VARIABLE(std::string)
INSTANTIATE_VARIABLE(double)
INSTANTIATE_VARIABLE(float)
INSTANTIATE_VARIABLE(int64_t)
INSTANTIATE_VARIABLE(int32_t)
INSTANTIATE_VARIABLE(bool)
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
INSTANTIATE_VARIABLE(scipp::index)
#endif
INSTANTIATE_VARIABLE(Eigen::Vector3d)
INSTANTIATE_VARIABLE(sparse_container<double>)
INSTANTIATE_VARIABLE(sparse_container<float>)
INSTANTIATE_VARIABLE(sparse_container<int64_t>)
INSTANTIATE_VARIABLE(sparse_container<int32_t>)
// Some sparse instantiations are only needed to avoid linker errors: Some
// makeVariable overloads have a runtime branch that may instantiate a sparse
// variable.
INSTANTIATE_VARIABLE(sparse_container<std::string>)
INSTANTIATE_VARIABLE(sparse_container<Bool>)
INSTANTIATE_VARIABLE(sparse_container<Eigen::Vector3d>)

INSTANTIATE_SET_VARIANCES(double)
INSTANTIATE_SET_VARIANCES(float)
INSTANTIATE_SET_VARIANCES(int64_t)
INSTANTIATE_SET_VARIANCES(int32_t)

template <class T1, class T2> bool equals(const T1 &a, const T2 &b) {
  if (!a || !b)
    return static_cast<bool>(a) == static_cast<bool>(b);
  if (a.unit() != b.unit())
    return false;
  return a.data() == b.data();
}

bool Variable::operator==(const Variable &other) const {
  return equals(*this, other);
}

bool Variable::operator==(const VariableConstProxy &other) const {
  return equals(*this, other);
}

bool Variable::operator!=(const Variable &other) const {
  return !(*this == other);
}

bool Variable::operator!=(const VariableConstProxy &other) const {
  return !(*this == other);
}

template <class T> VariableProxy VariableProxy::assign(const T &other) const {
  setUnit(other.unit());
  expect::equals(dims(), other.dims());
  data().copy(other.data(), Dim::Invalid, 0, 0, 1);
  return *this;
}

template SCIPP_CORE_EXPORT VariableProxy
VariableProxy::assign(const Variable &) const;
template SCIPP_CORE_EXPORT VariableProxy
VariableProxy::assign(const VariableConstProxy &) const;
template SCIPP_CORE_EXPORT VariableProxy
VariableProxy::assign(const VariableProxy &) const;

bool VariableConstProxy::operator==(const Variable &other) const {
  // Always use deep comparison (pointer comparison does not make sense since we
  // may be looking at a different section).
  return equals(*this, other);
}
bool VariableConstProxy::operator==(const VariableConstProxy &other) const {
  return equals(*this, other);
}

bool VariableConstProxy::operator!=(const Variable &other) const {
  return !(*this == other);
}
bool VariableConstProxy::operator!=(const VariableConstProxy &other) const {
  return !(*this == other);
}

void VariableProxy::setUnit(const units::Unit &unit) const {
  expectCanSetUnit(unit);
  m_mutableVariable->setUnit(unit);
}

void VariableProxy::expectCanSetUnit(const units::Unit &unit) const {
  if ((this->unit() != unit) && (dims() != m_mutableVariable->dims()))
    throw except::UnitError("Partial view on data of variable cannot be used "
                            "to change the unit.");
}

VariableConstProxy Variable::slice(const Slice slice) const & {
  return {*this, slice.dim(), slice.begin(), slice.end()};
}

Variable Variable::slice(const Slice slice) const && {
  return Variable{this->slice(slice)};
}

VariableProxy Variable::slice(const Slice slice) & {
  return {*this, slice.dim(), slice.begin(), slice.end()};
}

Variable Variable::slice(const Slice slice) && {
  return Variable{this->slice(slice)};
}

VariableConstProxy Variable::reshape(const Dimensions &dims) const & {
  return {*this, dims};
}

VariableProxy Variable::reshape(const Dimensions &dims) & {
  return {*this, dims};
}

Variable Variable::reshape(const Dimensions &dims) && {
  Variable reshaped(std::move(*this));
  reshaped.setDims(dims);
  return reshaped;
}

Variable VariableConstProxy::reshape(const Dimensions &dims) const {
  // In general a variable slice is not contiguous. Therefore we cannot reshape
  // without making a copy (except for special cases).
  Variable reshaped(*this);
  reshaped.setDims(dims);
  return reshaped;
}

void Variable::rename(const Dim from, const Dim to) {
  if (dims().contains(from))
    data().m_dimensions.relabel(dims().index(from), to);
}

// Example of a "derived" operation: Implementation does not require adding a
// virtual function to VariableConcept.
std::vector<Variable> split(const Variable &var, const Dim dim,
                            const std::vector<scipp::index> &indices) {
  if (indices.empty())
    return {var};
  std::vector<Variable> vars;
  vars.emplace_back(var.slice({dim, 0, indices.front()}));
  for (scipp::index i = 0; i < scipp::size(indices) - 1; ++i)
    vars.emplace_back(var.slice({dim, indices[i], indices[i + 1]}));
  vars.emplace_back(var.slice({dim, indices.back(), var.dims()[dim]}));
  return vars;
}

Variable concatenate(const VariableConstProxy &a1, const VariableConstProxy &a2,
                     const Dim dim) {
  if (a1.dtype() != a2.dtype())
    throw std::runtime_error(
        "Cannot concatenate Variables: Data types do not match.");
  if (a1.unit() != a2.unit())
    throw std::runtime_error(
        "Cannot concatenate Variables: Units do not match.");

  if (a1.dims().sparseDim() == dim && a2.dims().sparseDim() == dim) {
    Variable out(a1);
    transform_in_place<pair_self_t<sparse_container<double>>>(
        out, a2,
        overloaded{[](auto &a, const auto &b) {
                     a.insert(a.end(), b.begin(), b.end());
                   },
                   [](units::Unit &a, const units::Unit &b) {
                     expect::equals(a, b);
                   }});
    return out;
  }

  const auto &dims1 = a1.dims();
  const auto &dims2 = a2.dims();
  // TODO Many things in this function should be refactored and moved in class
  // Dimensions.
  // TODO Special handling for edge variables.
  if (dims1.sparseDim() != dims2.sparseDim())
    throw std::runtime_error("Cannot concatenate Variables: Either both or "
                             "neither must be sparse, and the sparse "
                             "dimensions must be the same.");
  for (const auto &dim1 : dims1.denseLabels()) {
    if (dim1 != dim) {
      if (!dims2.contains(dim1))
        throw std::runtime_error(
            "Cannot concatenate Variables: Dimensions do not match.");
      if (dims2[dim1] != dims1[dim1])
        throw std::runtime_error(
            "Cannot concatenate Variables: Dimension extents do not match.");
    }
  }
  auto size1 = dims1.shape().size();
  auto size2 = dims2.shape().size();
  if (dims1.contains(dim))
    size1--;
  if (dims2.contains(dim))
    size2--;
  // This check covers the case of dims2 having extra dimensions not present in
  // dims1.
  // TODO Support broadcast of dimensions?
  if (size1 != size2)
    throw std::runtime_error(
        "Cannot concatenate Variables: Dimensions do not match.");

  Variable out(a1);
  auto dims(dims1);
  scipp::index extent1 = 1;
  scipp::index extent2 = 1;
  if (dims1.contains(dim))
    extent1 += dims1[dim] - 1;
  if (dims2.contains(dim))
    extent2 += dims2[dim] - 1;
  if (dims.contains(dim))
    dims.resize(dim, extent1 + extent2);
  else
    dims.add(dim, extent1 + extent2);
  out.setDims(dims);

  out.data().copy(a1.data(), dim, 0, 0, extent1);
  out.data().copy(a2.data(), dim, extent1, 0, extent2);

  return out;
}

Variable permute(const Variable &var, const Dim dim,
                 const std::vector<scipp::index> &indices) {
  auto permuted(var);
  for (scipp::index i = 0; i < scipp::size(indices); ++i)
    permuted.data().copy(var.data(), dim, i, indices[i], indices[i] + 1);
  return permuted;
}

Variable filter(const Variable &var, const Variable &filter) {
  if (filter.dims().shape().size() != 1)
    throw std::runtime_error(
        "Cannot filter variable: The filter must by 1-dimensional.");
  const auto dim = filter.dims().labels()[0];
  auto mask = filter.values<bool>();

  const scipp::index removed = std::count(mask.begin(), mask.end(), 0);
  if (removed == 0)
    return var;

  auto out(var);
  auto dims = out.dims();
  dims.resize(dim, dims[dim] - removed);
  out.setDims(dims);

  scipp::index iOut = 0;
  // Note: Could copy larger chunks of applicable for better(?) performance.
  // Note: This implementation is inefficient, since we need to cast to concrete
  // type for *every* slice. Should be combined into a single virtual call.
  for (scipp::index iIn = 0; iIn < mask.size(); ++iIn)
    if (mask[iIn])
      out.data().copy(var.data(), dim, iOut++, iIn, iIn + 1);
  return out;
}

Variable sum(const VariableConstProxy &var, const Dim dim) {
  expect::notSparse(var);
  auto dims = var.dims();
  dims.erase(dim);
  Variable summed(var, dims);
  accumulate_in_place<
      pair_self_t<double, float, int64_t, int32_t, Eigen::Vector3d>>(
      summed, var, [](auto &&a, auto &&b) { a += b; });
  return summed;
}

struct multiply_variance {
  template <class T>
  scipp::core::detail::ValueAndVariance<T>
  operator()(const scipp::core::detail::ValueAndVariance<T> &vv) const {
    return {vv.value, vv.variance * multiplier};
  }
  double multiplier;
};

Variable mean(const VariableConstProxy &var, const Dim dim) {
  // In principle we *could* support mean/sum over sparse dimension.
  expect::notSparse(var);
  auto summed = sum(var, dim);
  double scale = 1.0 / static_cast<double>(var.dims()[dim]);
  if (isInt(var.dtype()))
    summed = summed * makeVariable<double>(scale);
  else
    summed *= makeVariable<double>(scale);
  if (summed.hasVariances())
    transform_in_place<double, float>(
        summed, overloaded{multiply_variance{scale}, [](const auto &) {}});
  return summed;
}

Variable abs(const Variable &var) {
  using std::abs;
  return transform<double, float>(var, [](const auto x) { return abs(x); });
}

Variable norm(const Variable &var) {
  return transform<Eigen::Vector3d>(
      var, overloaded{[](const auto &x) { return x.norm(); },
                      [](const units::Unit &x) { return x; }});
}

Variable sqrt(const Variable &var) {
  using std::sqrt;
  return transform<double, float>(var, [](const auto x) { return sqrt(x); });
}

Variable dot(const Variable &a, const Variable &b) {
  return transform<pair_self_t<Eigen::Vector3d>>(
      a, b,
      overloaded{[](const auto &a_, const auto &b_) { return a_.dot(b_); },
                 [](const units::Unit &a_, const units::Unit &b_) {
                   return a_ * b_;
                 }});
}

Variable broadcast(const VariableConstProxy &var, const Dimensions &dims) {
  if (var.dims().contains(dims))
    return Variable{var};
  auto newDims = var.dims();
  const auto labels = dims.labels();
  for (auto it = labels.end(); it != labels.begin();) {
    --it;
    const auto label = *it;
    if (newDims.contains(label))
      expect::dimensionMatches(newDims, label, dims[label]);
    else
      newDims.add(label, dims[label]);
  }
  Variable result(var);
  result.setDims(newDims);
  result.data().copy(var.data(), Dim::Invalid, 0, 0, 1);
  return result;
}

void swap(Variable &var, const Dim dim, const scipp::index a,
          const scipp::index b) {
  const Variable tmp(var.slice({dim, a}));
  var.slice({dim, a}).assign(var.slice({dim, b}));
  var.slice({dim, b}).assign(tmp);
}

Variable reverse(Variable var, const Dim dim) {
  const auto size = var.dims()[dim];
  for (scipp::index i = 0; i < size / 2; ++i)
    swap(var, dim, i, size - i - 1);
  return var;
}

template <>
VariableView<const double> getView<double>(const Variable &var,
                                           const Dimensions &dims) {
  return requireT<const VariableConceptT<double>>(var.data()).valuesView(dims);
}

} // namespace scipp::core
