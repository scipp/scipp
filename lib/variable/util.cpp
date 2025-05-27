// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/util.h"
#include "scipp/core/element/util.h"
#include "scipp/core/except.h"
#include "scipp/variable/accumulate.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

Variable linspace(const Variable &start, const Variable &stop, const Dim dim,
                  const scipp::index num) {
  // The implementation here is slightly verbose and explicit. It could be
  // improved if we were to introduce new variants of `transform`, similar to
  // `std::generate`.
  core::expect::equals(start.dims(), stop.dims());
  core::expect::equals(start.unit(), stop.unit());
  core::expect::equals(start.dtype(), stop.dtype());
  if (start.dtype() != dtype<double> && start.dtype() != dtype<float>)
    throw except::TypeError(
        "Cannot create linspace with non-floating-point start and/or stop.");
  if (start.has_variances() || stop.has_variances())
    throw except::VariancesError(
        "Cannot create linspace with start and/or stop containing variances.");
  auto dims = start.dims();
  dims.addInner(dim, num);
  Variable out(start, dims);
  const auto range = stop - start;
  for (scipp::index i = 0; i < num - 1; ++i)
    copy(start + astype(static_cast<double>(i) / (num - 1) * sc_units::one,
                        start.dtype()) *
                     range,
         out.slice({dim, i}));
  copy(stop, out.slice({dim, num - 1})); // endpoint included
  return out;
}

Variable islinspace(const Variable &var, const Dim dim) {
  const auto con_var = as_contiguous(var, dim);
  return transform(subspan_view(con_var, dim), core::element::islinspace,
                   "islinspace");
}

Variable isarange(const Variable &var, const Dim dim) {
  return transform(subspan_view(var, dim), core::element::isarange, "isarange");
}

/// Return a variable of True, if variable values are sorted along given dim.
///
/// If `order` is SortOrder::Ascending, checks if values are non-decreasing.
/// If `order` is SortOrder::Descending, checks if values are non-increasing.
Variable issorted(const Variable &x, const Dim dim, const SortOrder order) {
  auto dims = x.dims();
  dims.erase(dim);
  auto out = variable::ones(dims, sc_units::none, dtype<bool>);
  const auto size = x.dims()[dim];
  if (size < 2)
    return out;
  if (order == SortOrder::Ascending)
    accumulate_in_place(out, x.slice({dim, 0, size - 1}),
                        x.slice({dim, 1, size}),
                        core::element::issorted_nondescending, "issorted");
  else
    accumulate_in_place(out, x.slice({dim, 0, size - 1}),
                        x.slice({dim, 1, size}),
                        core::element::issorted_nonascending, "issorted");
  return out;
}

/// Return true if variable values are sorted along given dim.
///
/// If `order` is SortOrder::Ascending, checks if values are non-decreasing.
/// If `order` is SortOrder::Descending, checks if values are non-increasing.
bool allsorted(const Variable &x, const Dim dim, const SortOrder order) {
  return variable::all(issorted(x, dim, order)).value<bool>();
}

/// Zip elements of two variables into a variable where each element is a pair.
Variable zip(const Variable &first, const Variable &second) {
  return transform(astype(first, dtype<int64_t>, CopyPolicy::TryAvoid),
                   astype(second, dtype<int64_t>, CopyPolicy::TryAvoid),
                   core::element::zip, "zip");
}

/// For an input where elements are pairs, return two variables containing the
/// first and second components of the input pairs.
std::pair<Variable, Variable> unzip(const Variable &var) {
  return {var.elements<scipp::index_pair>("begin"),
          var.elements<scipp::index_pair>("end")};
}

/// Fill variable with given values (and variances) and unit.
void fill(Variable &var, const Variable &value) {
  transform_in_place(var, value, core::element::fill, "fill");
}

/// Fill variable with zeros.
void fill_zeros(Variable &var) {
  transform_in_place(var, core::element::fill_zeros, "fill_zeros");
}

/// Return elements chosen from x or y depending on condition.
Variable where(const Variable &condition, const Variable &x,
               const Variable &y) {
  return variable::transform(condition, x, y, element::where, "where");
}

Variable as_contiguous(const Variable &var, const Dim dim) {
  if (var.stride(dim) == 1)
    return var;
  auto dims = var.dims();
  dims.erase(dim);
  dims.addInner(dim, var.dims()[dim]);
  return copy(transpose(var, dims.labels()));
}

} // namespace scipp::variable
