// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>
#include <string>

#include "scipp/core/apply.h"
#include "scipp/core/counts.h"
#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
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

bool isMatchingOr1DBinEdge(const Dim dim, Dimensions edges,
                           const Dimensions &toMatch) {
  if (edges.shape().size() == 1)
    return true;
  edges.resize(dim, edges[dim] - 1);
  return edges == toMatch;
}

Variable::Variable(const Variable &parent, const Dimensions &dims)
    : m_unit(parent.unit()), m_object(parent.m_object->clone(dims)) {}

Variable::Variable(const VariableConstProxy &parent, const Dimensions &dims)
    : m_unit(parent.unit()), m_object(parent.data().clone(dims)) {}

Variable::Variable(const Variable &parent, VariableConceptHandle data)
    : m_unit(parent.unit()), m_object(std::move(data)) {}

void Variable::setDims(const Dimensions &dimensions) {
  if (dimensions.volume() == m_object->dims().volume()) {
    if (dimensions != m_object->dims())
      data().m_dimensions = dimensions;
    return;
  }
  m_object = m_object->clone(dimensions);
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
INSTANTIATE_VARIABLE(Dataset)
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
INSTANTIATE_VARIABLE(sparse_container<Dataset>)
INSTANTIATE_VARIABLE(sparse_container<Eigen::Vector3d>)

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
  if (dims() != other.dims())
    throw except::DimensionMismatchError(dims(), other.dims());
  data().copy(other.data(), Dim::Invalid, 0, 0, 1);
  return *this;
}

template VariableProxy VariableProxy::assign(const Variable &) const;
template VariableProxy VariableProxy::assign(const VariableConstProxy &) const;

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
  return {*this, slice.dim, slice.begin, slice.end};
}

Variable Variable::slice(const Slice slice) const && {
  return {this->slice(slice)};
}

VariableProxy Variable::slice(const Slice slice) & {
  return {*this, slice.dim, slice.begin, slice.end};
}

Variable Variable::slice(const Slice slice) && { return {this->slice(slice)}; }

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

Variable concatenate(const Variable &a1, const Variable &a2, const Dim dim) {
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
        [](auto &a, const auto &b) { a.insert(a.end(), b.begin(), b.end()); });
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

  auto out(a1);
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

Variable rebin(const Variable &var, const Variable &oldCoord,
               const Variable &newCoord) {
// TODO Disabled since it is using neutron-specific units. Should be moved
// into scipp-neutron? On the other hand, counts is actually more generic than
// neutron data, but requiring this unit to be part of all supported unit
// systems does not make sense either, I think.
#ifndef SCIPP_UNITS_NEUTRON
  throw std::runtime_error("rebin is disabled for this set of units");
#else
  expect::countsOrCountsDensity(var);
  Dim dim = Dim::Invalid;
  for (const auto d : oldCoord.dims().labels())
    if (oldCoord.dims()[d] == var.dims()[d] + 1) {
      dim = d;
      break;
    }

  auto do_rebin = [dim](auto &&out, auto &&old, auto &&oldCoord_,
                        auto &&newCoord_) {
    // Dimensions of *this and old are guaranteed to be the same.
    const auto &oldT = *old;
    const auto &oldCoordT = *oldCoord_;
    const auto &newCoordT = *newCoord_;
    auto &outT = *out;
    const auto &dims = outT.dims();
    if (dims.inner() == dim &&
        isMatchingOr1DBinEdge(dim, oldCoordT.dims(), oldT.dims()) &&
        isMatchingOr1DBinEdge(dim, newCoordT.dims(), dims)) {
      RebinHelper<typename std::remove_reference_t<decltype(
          outT)>::value_type>::rebinInner(dim, oldT, outT, oldCoordT,
                                          newCoordT);
    } else {
      throw std::runtime_error(
          "TODO the new coord should be 1D or the same dim as newCoord.");
    }
  };

  if (var.unit() == units::counts) {
    auto dims = var.dims();
    dims.resize(dim, newCoord.dims()[dim] - 1);
    Variable rebinned(var, dims);
    if (rebinned.dims().inner() == dim) {
      apply_in_place<double, float>(do_rebin, rebinned, var, oldCoord,
                                    newCoord);
    } else {
      if (newCoord.dims().shape().size() > 1)
        throw std::runtime_error(
            "Not inner rebin works only for 1d coordinates for now.");
      switch (rebinned.dtype()) {
      case dtype<double>:
        RebinGeneralHelper<double>::rebin(dim, var, rebinned, oldCoord,
                                          newCoord);
        break;
      case dtype<float>:
        RebinGeneralHelper<float>::rebin(dim, var, rebinned, oldCoord,
                                         newCoord);
        break;
      default:
        throw std::runtime_error(
            "Rebinning is possible only for double and float types.");
      }
    }
    return rebinned;
  } else {
    // TODO This will currently fail if the data is a multi-dimensional density.
    // Would need a conversion that converts only the rebinned dimension.
    // TODO This could be done more efficiently without a temporary Dataset.
    throw std::runtime_error("Temporarily disabled for refactor");
    /*
    Dataset density;
    density.insert(dimensionCoord(dim), oldCoord);
    density.insert(Data::Value, var);
    auto cnts = counts::fromDensity(std::move(density), dim).erase(Data::Value);
    Dataset rebinnedCounts;
    rebinnedCounts.insert(dimensionCoord(dim), newCoord);
    rebinnedCounts.insert(Data::Value,
                          rebin(std::get<Variable>(cnts), oldCoord, newCoord));
    return std::get<Variable>(
        counts::toDensity(std::move(rebinnedCounts), dim).erase(Data::Value));
    */
  }
#endif
}

Variable permute(const Variable &var, const Dim dim,
                 const std::vector<scipp::index> &indices) {
  auto permuted(var);
  for (size_t i = 0; i < indices.size(); ++i)
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

Variable sum(const Variable &var, const Dim dim) {
  auto summed(var);
  auto dims = summed.dims();
  dims.erase(dim);
  // setDims zeros the data
  summed.setDims(dims);
  transform_in_place<pair_self_t<double, float, int64_t, Eigen::Vector3d>>(
      summed, var, [](auto &&a, auto &&b) { a += b; });
  return summed;
}

Variable mean(const Variable &var, const Dim dim) {
  auto summed = sum(var, dim);
  double scale = 1.0 / static_cast<double>(var.dims()[dim]);
  return summed * makeVariable<double>(scale);
}

Variable abs(const Variable &var) {
  using std::abs;
  return transform<double, float>(var, [](const auto x) { return abs(x); });
}

Variable norm(const Variable &var) {
  return transform<Eigen::Vector3d>(var, [](auto &&x) { return x.norm(); });
}

Variable sqrt(const Variable &var) {
  using std::sqrt;
  Variable result =
      transform<double, float>(var, [](const auto x) { return sqrt(x); });
  result.setUnit(sqrt(var.unit()));
  return result;
}

Variable broadcast(Variable var, const Dimensions &dims) {
  if (var.dims().contains(dims))
    return var;
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
  const Variable tmp = var.slice({dim, a});
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
