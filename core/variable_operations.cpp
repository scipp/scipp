// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/core/apply.h"
#include "scipp/core/dtype.h"
#include "scipp/core/except.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

#include "element_unary_operations.h"
#include "operators.h"
#include "variable_operations_common.h"

namespace scipp::core {

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

Variable reciprocal(const VariableConstView &var) {
  return transform<double, float>(
      var,
      overloaded{
          [](const auto &a_) {
            return static_cast<
                       detail::element_type_t<std::decay_t<decltype(a_)>>>(1) /
                   a_;
          },
          [](const units::Unit &unit) {
            return units::Unit(units::dimensionless) / unit;
          }});
}

Variable reciprocal(Variable &&var) {
  auto out(std::move(var));
  reciprocal(out, out);
  return out;
}

VariableView reciprocal(const VariableConstView &var, const VariableView &out) {
  transform_in_place<pair_self_t<double, float>>(
      out, var,
      overloaded{
          [](auto &x, const auto &y) {
            x = static_cast<detail::element_type_t<std::decay_t<decltype(y)>>>(
                    1) /
                y;
          },
          [](units::Unit &x, const units::Unit &y) {
            x = units::Unit(units::dimensionless) / y;
          }});
  return out;
}

Variable abs(const VariableConstView &var) {
  using std::abs;
  return transform<double, float>(var, [](const auto x) { return abs(x); });
}

Variable abs(Variable &&var) {
  using std::abs;
  auto out(std::move(var));
  abs(out, out);
  return out;
}

VariableView abs(const VariableConstView &var, const VariableView &out) {
  using std::abs;
  transform_in_place<pair_self_t<double, float>>(
      out, var, [](auto &x, const auto &y) { x = abs(y); });
  return out;
}

Variable norm(const VariableConstView &var) {
  return transform<Eigen::Vector3d>(
      var, overloaded{[](const auto &x) { return x.norm(); },
                      [](const units::Unit &x) { return x; }});
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

Variable dot(const Variable &a, const Variable &b) {
  return transform<pair_self_t<Eigen::Vector3d>>(
      a, b,
      overloaded{[](const auto &a_, const auto &b_) { return a_.dot(b_); },
                 [](const units::Unit &a_, const units::Unit &b_) {
                   return a_ * b_;
                 }});
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
  using std::isnan;
  transform_in_place<std::tuple<double, float>>(
      out, var, replacement,
      scipp::overloaded{
          transform_flags::expect_all_or_none_have_variance,
          [](auto &a, const auto &b, const auto &repl) {
            a = isnan(b) ? repl : b;
          },
          [](units::Unit &a, const units::Unit &b, const units::Unit &repl) {
            expect::equals(b, repl);
            a = b;
          }});
  return out;
}

Variable nan_to_num(const VariableConstView &var,
                    const VariableConstView &replacement) {
  using std::isnan;
  return transform<std::tuple<double, float>>(
      var, replacement,
      overloaded{
          transform_flags::expect_all_or_none_have_variance,
          [](const auto &x, const auto &repl) { return isnan(x) ? repl : x; },
          [](const units::Unit &x, const units::Unit &repl) {
            expect::equals(x, repl);
            return x;
          }});
}

} // namespace scipp::core
