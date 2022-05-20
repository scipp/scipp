// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/string.h"
#include "scipp/variable/bin_array_variable.tcc"
#include "scipp/variable/bins.h"
#include "scipp/variable/string.h"

namespace scipp::variable {

INSTANTIATE_BIN_ARRAY_VARIABLE(DatasetView, Dataset)
INSTANTIATE_BIN_ARRAY_VARIABLE(DataArrayView, DataArray)

} // namespace scipp::variable

namespace scipp::dataset {

namespace {
Variable apply_mask(const DataArray &buffer, const Variable &indices,
                    const Dim dim, const Variable &mask, const FillValue fill) {
  return make_bins(
      indices, dim,
      where(mask, special_like(Variable(buffer.data(), Dimensions{}), fill),
            buffer.data()));
}
} // namespace

class BinVariableMakerDataArray : public variable::BinVariableMaker<DataArray> {
private:
  Variable call_make_bins(const Variable &parent, const Variable &indices,
                          const Dim dim, const DType type,
                          const Dimensions &dims, const units::Unit &unit,
                          const bool variances) const override {
    const auto &source = buffer(parent);
    if (parent.dims() !=
        indices
            .dims()) // would need to select and copy slices from source coords
      throw std::runtime_error(
          "Shape changing operations with bucket<DataArray> not supported yet");
    // TODO This may also fail if the input buffer has extra capacity (rows not
    // in any bucket).
    auto buffer = DataArray(
        variable::variableFactory().create(type, dims, unit, variances),
        copy(source.coords()), copy(source.masks()), copy(source.attrs()));
    // TODO is the copy needed?
    return make_bins(copy(indices), dim, std::move(buffer));
  }
  const Variable &data(const Variable &var) const override {
    return buffer(var).data();
  }
  Variable data(Variable &var) const override { return buffer(var).data(); }

  [[nodiscard]] Variable
  apply_event_masks(const Variable &var, const FillValue fill) const override {
    if (var.dtype() == dtype<bucket<dataset::DataArray>>) {
      const auto &&[indices, dim, buffer] = var.constituents<DataArray>();
      if (const auto mask_union = irreducible_mask(buffer.masks(), dim);
          mask_union.is_valid()) {
        return apply_mask(buffer, indices, dim, mask_union, fill);
      }
    }
    return var;
  }
};

/// This is currently a dummy implemented just to make `is_bins` work.
class BinVariableMakerDataset
    : public variable::BinVariableMakerCommon<Dataset> {
  Variable create(const DType, const Dimensions &, const units::Unit &,
                  const bool, const parent_list &) const override {
    throw std::runtime_error("not implemented");
  }
  Dim elem_dim(const Variable &) const override {
    throw std::runtime_error("undefined");
  }
  DType elem_dtype(const Variable &) const override {
    throw std::runtime_error("undefined");
  }
  units::Unit elem_unit(const Variable &) const override {
    throw std::runtime_error("undefined");
  }
  void expect_can_set_elem_unit(const Variable &,
                                const units::Unit &) const override {
    throw std::runtime_error("undefined");
  }
  void set_elem_unit(Variable &, const units::Unit &) const override {
    throw std::runtime_error("undefined");
  }
  bool has_variances(const Variable &) const override {
    throw std::runtime_error("undefined");
  }
  [[nodiscard]] Variable apply_event_masks(const Variable &,
                                           const FillValue) const override {
    throw except::NotImplementedError(
        "Event masks for bins containing datasets are not supported.");
  }
};

REGISTER_FORMATTER(bin_DataArray, core::bin<DataArray>)
REGISTER_FORMATTER(bin_Dataset, core::bin<Dataset>)

namespace {
auto register_variable_maker_bucket_DataArray(
    (variable::variableFactory().emplace(
         dtype<bucket<DataArray>>,
         std::make_unique<BinVariableMakerDataArray>()),
     variable::variableFactory().emplace(
         dtype<bucket<Dataset>>, std::make_unique<BinVariableMakerDataset>()),
     0));
} // namespace
} // namespace scipp::dataset
