
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold

#include "scipp/core/dimensions.h"
#include "scipp/core/variable.h"
#include "scipp/core/variable_view.h"
#include "scipp/core/vector.h"
#include "scipp/units/unit.h"

namespace scipp::core {
/**
  Support explicit instantionatons for templates
  */
#define INSTANTIATE_VARIABLE(...)                                              \
  template Variable::Variable(const units::Unit, const Dimensions &,           \
                              Vector<underlying_type_t<__VA_ARGS__>>);         \
  template Variable::Variable(const units::Unit, const Dimensions &,           \
                              Vector<underlying_type_t<__VA_ARGS__>>,          \
                              Vector<underlying_type_t<__VA_ARGS__>>);         \
  template Vector<underlying_type_t<__VA_ARGS__>>                              \
      &Variable::cast<__VA_ARGS__>(const bool);                                \
  template const Vector<underlying_type_t<__VA_ARGS__>>                        \
      &Variable::cast<__VA_ARGS__>(const bool) const;
}

#define INSTANTIATE_SLICEVIEW(...)                                             \
  template const VariableView<const underlying_type_t<__VA_ARGS__>>            \
  VariableConstProxy::cast<__VA_ARGS__>() const;                               \
  template const VariableView<const underlying_type_t<__VA_ARGS__>>            \
  VariableConstProxy::castVariances<__VA_ARGS__>() const;                      \
  template VariableView<underlying_type_t<__VA_ARGS__>>                        \
  VariableProxy::cast<__VA_ARGS__>() const;                                    \
  template VariableView<underlying_type_t<__VA_ARGS__>>                        \
  VariableProxy::castVariances<__VA_ARGS__>() const;
