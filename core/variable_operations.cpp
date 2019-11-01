// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/core/apply.h"
#include "scipp/core/counts.h"
#include "scipp/core/dtype.h"
#include "scipp/core/except.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

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

struct disable_variance_first_arg {
  template <class First, class Second>
  void operator()(const scipp::core::detail::ValueAndVariance<First>,
                  const Second &) const {
    throw except::VariancesError("Variances in first argument not supported.");
  }
};

struct disable_variance_second_arg {
  template <class First, class Second>
  void operator()(const First &,
                  const scipp::core::detail::ValueAndVariance<Second>) const {
    throw except::VariancesError("Variances in second argument not supported.");
  }
};

namespace sparse {
/// Return array of sparse dimension extents, i.e., total counts.
Variable counts(const VariableConstProxy &var) {
  // To simplify this we would like to use `transform`, but this is currently
  // not possible since the current implementation expects outputs with
  // variances if any of the inputs has variances.
  auto dims = var.dims();
  dims.erase(dims.sparseDim());
  auto counts = makeVariable<scipp::index>(dims, units::counts);
  accumulate_in_place<
      pair_custom_t<std::pair<scipp::index, sparse_container<double>>>>(
      counts, var,
      overloaded{[](scipp::index &c, const auto &sparse) { c = sparse.size(); },
                 disable_variance_first_arg{},
                 transform_flags::no_variance_output});
  return counts;
}

/// Reserve memory in all sparse containers in `sparse`, based on `capacity`.
void reserve(const VariableProxy &sparse, const VariableConstProxy &capacity) {
  transform_in_place<
      pair_custom_t<std::pair<sparse_container<double>, scipp::index>>>(
      sparse, capacity,
      overloaded{[](auto &&sparse_, const scipp::index capacity_) {
                   return sparse_.reserve(capacity_);
                 },
                 disable_variance_second_arg{},
                 [](const units::Unit &, const units::Unit &) {}});
}
} // namespace sparse

void flatten_impl(const VariableProxy &summed, const VariableConstProxy &var) {
  // 1. Reserve space in output. This yields approx. 3x speedup.
  auto summed_counts = sparse::counts(summed);
  sum_impl(summed_counts, sparse::counts(var));
  sparse::reserve(summed, summed_counts);

  // 2. Flatten dimension(s) by concatenating along sparse dim.
  accumulate_in_place<pair_self_t<sparse_container<double>>,
                      pair_self_t<sparse_container<float>>,
                      pair_self_t<sparse_container<int64_t>>,
                      pair_self_t<sparse_container<int32_t>>>(
      summed, var,
      overloaded{
          [](auto &a, const auto &b) { a.insert(a.end(), b.begin(), b.end()); },
          [](units::Unit &a, const units::Unit &b) { expect::equals(a, b); }});
}

void sum_impl(const VariableProxy &summed, const VariableConstProxy &var) {
  accumulate_in_place<
      pair_self_t<double, float, int64_t, int32_t, Eigen::Vector3d>,
      pair_custom_t<std::pair<int64_t, bool>>>(
      summed, var, [](auto &&a, auto &&b) { a += b; });
}

Variable sum(const VariableConstProxy &var, const Dim dim) {
  expect::notSparse(var);
  auto dims = var.dims();
  dims.erase(dim);
  // Bool DType is a bit special in that it cannot contain it's sum.
  // Instead the sum is stored in a int64_t Variable
  Variable summed{var.dtype() == DType::Bool ? makeVariable<int64_t>(dims)
                                             : Variable(var, dims)};
  sum_impl(summed, var);
  return summed;
}

Variable sum(const VariableConstProxy &var, const Dim dim,
             const MasksConstProxy &masks) {
  if (masks.empty()) {
    return sum(var, dim);
  } else {
    const auto mask_union = masks_merge(masks, var.dims());
    return sum(var * ~mask_union, dim);
  }
}

Variable mean(const VariableConstProxy &var, const Dim dim) {
  return mean(var, dim, makeVariable<int64_t>(0));
}

Variable mean(const VariableConstProxy &var, const Dim dim,
              const VariableConstProxy &masks_sum) {
  // In principle we *could* support mean/sum over sparse dimension.
  expect::notSparse(var);
  auto summed = sum(var, dim);

  auto scale = 1.0 / (makeVariable<double>(var.dims()[dim]) - masks_sum);

  if (isInt(var.dtype()))
    summed = summed * scale;
  else
    summed *= scale;
  return summed;
}

Variable mean(const VariableConstProxy &var, const Dim dim,
              const MasksConstProxy &masks) {
  if (masks.empty()) {
    return mean(var, dim);
  } else {
    const auto mask_union = masks_merge(masks, var.dims());
    const auto masks_sum = sum(mask_union, dim);
    return mean(var * ~mask_union, dim, masks_sum);
  }
}

Variable abs(const Variable &var) {
  using std::abs;
  return transform<double, float>(var, [](const auto x) { return abs(x); });
}

Variable norm(const VariableConstProxy &var) {
  return transform<Eigen::Vector3d>(
      var, overloaded{[](const auto &x) { return x.norm(); },
                      [](const units::Unit &x) { return x; }});
}

Variable sqrt(const VariableConstProxy &var) {
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

Variable resize(const VariableConstProxy &var, const Dim dim,
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

/// Return a deep copy of a Variable or of a VariableProxy.
Variable copy(const VariableConstProxy &var) { return Variable(var); }

Variable masks_merge(const MasksConstProxy &masks, const Dimensions dims) {
  auto mask_union = makeVariable<bool>(dims);
  for (const auto &mask : masks) {
    mask_union |= mask.second;
  }
  return mask_union;
}
} // namespace scipp::core
