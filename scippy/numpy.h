// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPPY_NUMPY_H
#define SCIPPY_NUMPY_H

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

template <typename T, typename SZ_TP>
Variable makeVariableFromBuffer(const Dimensions &dimensions,
                                const std::vector<SZ_TP> &stridesInBytes,
                                T *ptr) {
  auto ndims = scipp::size(dimensions.shape());
  if (ndims == 0) {
    throw std::runtime_error(
        "bug in old implementation, need to init single element!.");
    return makeVariable<underlying_type_t<T>>(dimensions);
  }

  std::vector<SZ_TP> varStrides(ndims, 1), strides;
  for (auto &&strd : stridesInBytes)
    strides.emplace_back(strd / sizeof(T));

  bool sameStrides{*strides.rbegin() == 1};
  auto i = scipp::size(varStrides) - 1;
  while (i-- > 0) {
    varStrides[i] = varStrides[i + 1] * dimensions.size(i + 1);
    if (varStrides[i] != strides[i] && sameStrides)
      sameStrides = false;
  }

  if (sameStrides) { // memory is alligned c-style and dense
    return Variable(
        units::dimensionless, std::move(dimensions),
        Vector<underlying_type_t<T>>(ptr, ptr + dimensions.volume()));

  } else {
    // Try to find blocks to copy
    auto index = scipp::size(strides) - 1;
    while (strides[index] == varStrides[index])
      --index;
    ++index;
    auto blockSz = index < scipp::size(strides)
                       ? strides[index] * dimensions.size(index)
                       : 1;

    auto res = makeVariable<underlying_type_t<T>>(dimensions);
    std::vector<scipp::index> dsz(ndims);
    for (scipp::index i = 0; i < index; ++i)
      dsz[i] = dimensions.size(i);
    std::vector<scipp::index> coords(ndims, 0);
    auto nBlocks = dimensions.volume() / blockSz;

    for (scipp::index i = 0; i < nBlocks; ++i) {
      // calculate the array linear coordinate
      auto lin_coord = std::inner_product(coords.begin(), coords.end(),
                                          strides.begin(), scipp::index{0});
      std::memcpy(&res.template values<T>()[i * blockSz], &ptr[lin_coord],
                  blockSz * sizeof(T));
      // get the next ND coordinate
      auto k = coords.size();
      while (k-- > 0)
        ++coords[k] >= dsz[k] ? coords[k] = 0 : k = 0;
    }
    return res;
  }
}

#endif // SCIPPY_NUMPY_H
