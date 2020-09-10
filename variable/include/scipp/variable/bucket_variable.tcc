// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

template <class T>
std::tuple<VariableConstView, Dim, typename T::const_element_type>
VariableConstView::constituents() const {
  auto view = *this;
  const auto &model = requireT<const DataModel<T>>(underlying().data());
  view.m_variable = &model.indices();
  return {view, model.dim(), model.buffer()};
}

/// Macro for instantiating classes and functions required for support a new
/// bucket dtype in Variable.
#define INSTANTIATE_BUCKET_VARIABLE(name, ...)                                 \
  INSTANTIATE_VARIABLE_BASE(name, __VA_ARGS__)                                 \
  template std::tuple<VariableConstView, Dim,                                  \
                      typename __VA_ARGS__::const_element_type>                \
  VariableConstView::constituents<__VA_ARGS__>() const;

} // namespace scipp::variable
