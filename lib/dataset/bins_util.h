// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/bins.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/util.h"

namespace scipp::dataset {

template <class Masks>
Variable hide_masked(const Variable &data, const Masks &masks,
                     const std::span<const Dim> dims) {
  const auto empty_range = makeVariable<scipp::index_pair>(
      Values{std::pair<scipp::index, scipp::index>(0, 0)});
  const auto &[begin_end, buffer_dim, buffer] = data.constituents<DataArray>();
  auto indices = begin_end;
  for (const auto dim : dims) {
    auto mask = irreducible_mask(masks, dim);
    if (mask.is_valid()) {
      mask = transpose(mask, intersection(data.dims(), mask.dims()).labels());
      indices = where(mask, empty_range, indices);
    }
  }
  return make_bins_no_validate(indices, buffer_dim, buffer);
}

} // namespace scipp::dataset
