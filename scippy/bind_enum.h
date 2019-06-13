// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPPY_BIND_ENUM_H
#define SCIPPY_BIND_ENUM_H

#include "pybind11.h"

template <class Enum>
void bind_enum(pybind11::module &m, const std::string &name, const Enum last,
               const size_t stripPrefix = 0) {
  pybind11::enum_<Enum> en(m, name.c_str());
  for (int32_t i = 0; i <= static_cast<int32_t>(last); ++i) {
    const auto val = static_cast<Enum>(i);
    en.value(to_string(val).substr(stripPrefix).c_str(), val);
  }
}

#endif // SCIPPY_BIND_ENUM_H
