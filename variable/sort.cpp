// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Thibault Chatel
#include <numeric>

#include "scipp/core/tag_util.h"
#include "scipp/variable/sort.h"
#include "scipp/variable/indexed_slice_view.h"

using scipp::variable::IndexedSliceView;

namespace scipp::variable {

template <class T> struct MakeSort {
  static auto apply(const VariableConstView &var, const Dim &key) {
  if (var.dims().ndim() == 1) {
    const auto &values = var.values<T>();
    std::vector<scipp::index> permutation(values.size());
    std::iota(permutation.begin(), permutation.end(), 0);
    std::sort(
          permutation.begin(), permutation.end(),
          [&](scipp::index i, scipp::index j) { return values[i] < values[j]; });
    return concatenate(
          IndexedSliceView{var, key, permutation});
  } else {
    scipp::index good_index = 0;
    Variable for_taille = Variable(var, var.dims());
    scipp::index taille = for_taille.dims().shape()[good_index];
    Dim y = var.dims().label(good_index);
    VariableConstView pre_viewtest(var, y, 0);
    Variable sorted = apply(pre_viewtest, key);
    for (scipp::index i = 1; i<taille; i++) {
      VariableConstView viewtest(var, y, i);
      Variable viewtest_trie = apply(viewtest, key);
      sorted = concatenate(sorted, viewtest_trie, y);
    }
    sorted.setDims(var.dims());

    return sorted;
  }
}
};


Variable sort(const VariableConstView &var, const Dim &key) {
  uint16_t ndim = var.dims().ndim();
  VariableConstView input = var;
  if (ndim>1) {
    scipp::index key_one = 0;
    for (scipp::index i = 0; i<ndim; i++) {
      if (key == (var.dims().label(i))) { key_one = i; }
    }
    if (key_one != (ndim-1)) {
      std::vector<Dim> for_transpose = {};
      for (scipp::index i = 0; i<ndim; i++) {
        if (i==key_one) { for_transpose.push_back(var.dims().label(ndim-1)); }
        else if (i==ndim-1) { for_transpose.push_back(var.dims().label(key_one)); }
        else { for_transpose.push_back(var.dims().label(i)); }
      }
      input = var.transpose(for_transpose);
    }
  }
  return core::CallDType<double, float, int64_t, int32_t, bool,
                         std::string>::apply<MakeSort>(input.dtype(), input, key);
}



} // namespace scipp::variable
