// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bin_variable.tcc"
#include "scipp/variable/bins.h"

namespace scipp::variable {

INSTANTIATE_VARIABLE(pair_int64, std::pair<int64_t, int64_t>)
INSTANTIATE_VARIABLE(pair_int32, std::pair<int32_t, int32_t>)
INSTANTIATE_BIN_VARIABLE(VariableView, bucket<Variable>)

template <class T> class BinVariableMakerVariable : public BinVariableMaker<T> {
private:
  Variable call_make_bins(const Variable &, const Variable &indices,
                          const Dim dim, const DType type,
                          const Dimensions &dims, const units::Unit &unit,
                          const bool variances) const override {
    // Buffer contains only variable, which is created with new dtype, no
    // information to copy from parent.
    return make_bins(copy(indices), dim,
                     variableFactory().create(type, dims, unit, variances));
  }
  const Variable &data(const Variable &var) const override {
    return this->buffer(var);
  }
  Variable data(Variable &var) const override { return this->buffer(var); }
};

void expect_valid_bin_indices(const VariableConceptHandle &indices,
                              const Dim dim, const Sizes &buffer_sizes) {
  auto copy = requireT<const ElementArrayModel<scipp::index_pair>>(*indices);
  const auto vals = copy.values();
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

namespace {
auto register_variable_maker_bucket_Variable(
    (variableFactory().emplace(
         dtype<bucket<Variable>>,
         std::make_unique<BinVariableMakerVariable<Variable>>()),
     0));
}

} // namespace scipp::variable
