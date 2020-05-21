// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/core/dtype.h"
#include "scipp/core/element/geometric_operations.h"
#include "scipp/core/element/unary_operations.h"
#include "scipp/variable/apply.h"
#include "scipp/variable/except.h"
#include "scipp/variable/misc_operations.h"
#include "scipp/variable/transform.h"

#include "operations_common.h"

using namespace scipp::core;

namespace scipp::variable {

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

Variable concatenate(const VariableConstView &a1, const VariableConstView &a2,
                     const Dim dim) {
  if (a1.dtype() != a2.dtype())
    throw std::runtime_error(
        "Cannot concatenate Variables: Data types do not match.");
  if (a1.unit() != a2.unit())
    throw std::runtime_error(
        "Cannot concatenate Variables: Units do not match.");

  const auto &dims1 = a1.dims();
  const auto &dims2 = a2.dims();
  // TODO Many things in this function should be refactored and moved in class
  // Dimensions.
  // TODO Special handling for edge variables.
  for (const auto &dim1 : dims1.labels()) {
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

  const scipp::index removed = std::count(mask.begin(), mask.end(), false);
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
  for (scipp::index iIn = 0; iIn < scipp::size(mask); ++iIn)
    if (mask[iIn])
      out.data().copy(var.data(), dim, iOut++, iIn, iIn + 1);
  return out;
}

Variable reciprocal(const VariableConstView &var) {
  return transform(var, element::reciprocal);
}

Variable reciprocal(Variable &&var) {
  auto out(std::move(var));
  reciprocal(out, out);
  return out;
}

VariableView reciprocal(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, element::reciprocal_out_arg);
  return out;
}

Variable abs(const VariableConstView &var) {
  return transform<double, float>(var, element::abs);
}

Variable abs(Variable &&var) {
  abs(var, var);
  return std::move(var);
}

VariableView abs(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, element::abs_out_arg);
  return out;
}

Variable norm(const VariableConstView &var) {
  return transform(var, element::norm);
}

Variable sqrt(const VariableConstView &var) {
  return transform<double, float>(var, element::sqrt);
}

Variable sqrt(Variable &&var) {
  sqrt(var, var);
  return std::move(var);
}

VariableView sqrt(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, element::sqrt_out_arg);
  return out;
}

Variable dot(const VariableConstView &a, const VariableConstView &b) {
  return transform(a, b, element::dot);
}

Variable broadcast(const VariableConstView &var, const Dimensions &dims) {
  if (var.dims().contains(dims))
    return Variable{var};
  auto newDims = var.dims();
  const auto labels = dims.labels();
  for (auto it = labels.end(); it != labels.begin();) {
    --it;
    const auto label = *it;
    if (newDims.contains(label))
      core::expect::dimensionMatches(newDims, label, dims[label]);
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

Variable resize(const VariableConstView &var, const Dim dim,
                const scipp::index size) {
  auto dims = var.dims();
  dims.resize(dim, size);
  return Variable(var, dims);
}

Variable reverse(Variable var, const Dim dim) {
  const auto size = var.dims()[dim];
  for (scipp::index i = 0; i < size / 2; ++i)
    swap(var, dim, i, size - i - 1);
  return var;
}

/// Return a deep copy of a Variable or of a VariableView.
Variable copy(const VariableConstView &var) { return Variable(var); }

VariableView nan_to_num(const VariableConstView &var,
                        const VariableConstView &replacement,
                        const VariableView &out) {
  transform_in_place<std::tuple<double, float>>(out, var, replacement,
                                                element::nan_to_num_out_arg);
  return out;
}

VariableView positive_inf_to_num(const VariableConstView &var,
                                 const VariableConstView &replacement,
                                 const VariableView &out) {
  transform_in_place<std::tuple<double, float>>(
      out, var, replacement, element::positive_inf_to_num_out_arg);
  return out;
}
VariableView negative_inf_to_num(const VariableConstView &var,
                                 const VariableConstView &replacement,
                                 const VariableView &out) {
  transform_in_place<std::tuple<double, float>>(
      out, var, replacement, element::negative_inf_to_num_out_arg);

  return out;
}

Variable nan_to_num(const VariableConstView &var,
                    const VariableConstView &replacement) {

  return transform<std::tuple<double, float>>(var, replacement,
                                              element::nan_to_num);
}

Variable pos_inf_to_num(const VariableConstView &var,
                        const VariableConstView &replacement) {

  return transform<std::tuple<double, float>>(var, replacement,
                                              element::positive_inf_to_num);
}

Variable neg_inf_to_num(const VariableConstView &var,
                        const VariableConstView &replacement) {
  return transform<std::tuple<double, float>>(var, replacement,
                                              element::negative_inf_to_num);
}

namespace geometry {
Variable position(const VariableConstView &x, const VariableConstView &y,
                  const VariableConstView &z) {
  return transform<std::tuple<double>>(x, y, z, element::geometry::position);
}
Variable x(const VariableConstView &pos) {
  return transform<std::tuple<Eigen::Vector3d>>(pos, element::geometry::x);
}
Variable y(const VariableConstView &pos) {
  return transform<std::tuple<Eigen::Vector3d>>(pos, element::geometry::y);
}
Variable z(const VariableConstView &pos) {
  return transform<std::tuple<Eigen::Vector3d>>(pos, element::geometry::z);
}
Variable rotate(const VariableConstView &pos, const VariableConstView &rot) {
  return transform(pos, rot, element::geometry::rotate);
}
VariableView rotate(const VariableConstView &pos, const VariableConstView &rot,
                    const VariableView &out) {
  transform_in_place(out, pos, rot, element::geometry::rotate_out_arg);
  return out;
}

} // namespace geometry

} // namespace scipp::variable
