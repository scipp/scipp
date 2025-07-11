// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include <algorithm>
#include <iterator>
#include <vector>

#include "scipp/core/dimensions.h"
#include "scipp/units/dim.h"

inline auto to_dim_type(const std::span<const std::string> dim_strings) {
  std::vector<scipp::Dim> dims;
  dims.reserve(dim_strings.size());
  std::transform(begin(dim_strings), end(dim_strings), std::back_inserter(dims),
                 [](const std::string &d) { return scipp::Dim{d}; });
  return dims;
}

inline scipp::core::Dimensions
make_dims(const std::span<const std::string> dim_strings,
          std::span<const scipp::index> shape) {
  const auto dims = to_dim_type(dim_strings);
  return scipp::core::Dimensions{dims, shape};
}
