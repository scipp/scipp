// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bin_array_variable.tcc"
#include "scipp/variable/bins.h"

#include "operations_common.h"

namespace scipp::variable {

template <>
std::string Formatter<core::bin<Variable>>::format(const Variable &var) const {
  const auto &[indices, dim, content] = var.constituents<Variable>();
  return "binned data: dim='" + to_string(dim) + "', content=Variable" +
         format_variable_like(content);
}

INSTANTIATE_BIN_ARRAY_VARIABLE(VariableView, Variable)

template <class T> class BinVariableMakerVariable : public BinVariableMaker<T> {
private:
  Variable call_make_bins(const Variable &, const Variable &indices,
                          const Dim dim, const DType type,
                          const Dimensions &dims, const sc_units::Unit &unit,
                          const bool variances) const override {
    // Buffer contains only variable, which is created with new dtype, no
    // information to copy from parent.
    return make_bins_no_validate(
        indices, dim, variableFactory().create(type, dims, unit, variances));
  }
  const Variable &data(const Variable &var) const override {
    return this->buffer(var);
  }
  Variable data(Variable &var) const override { return this->buffer(var); }
};

void expect_valid_bin_indices(const Variable &indices, const Dim dim,
                              const Sizes &buffer_sizes) {
  core::expect::equals(sc_units::none, indices.unit());
  auto var = copy(indices);
  const auto vals = var.values<scipp::index_pair>().as_span();
  std::sort(vals.begin(), vals.end());
  if ((!vals.empty() && (vals.begin()->first < 0)) ||
      (!vals.empty() && ((vals.end() - 1)->second > buffer_sizes[dim])))
    throw except::SliceError("Bin indices out of range");
  if (std::adjacent_find(vals.begin(), vals.end(),
                         [](const auto a, const auto b) {
                           return a.second > b.first;
                         }) != vals.end())
    throw except::SliceError("Overlapping bin indices are not allowed.");
  if (std::find_if(vals.begin(), vals.end(), [](const auto x) {
        return x.first > x.second;
      }) != vals.end())
    throw except::SliceError(
        "Bin begin index must be less or equal to its end index.");
}

REGISTER_FORMATTER(bin_Variable, core::bin<Variable>)

namespace {
auto register_variable_maker_bucket_Variable(
    (variableFactory().emplace(
         dtype<bucket<Variable>>,
         std::make_unique<BinVariableMakerVariable<Variable>>()),
     0));
}

} // namespace scipp::variable
