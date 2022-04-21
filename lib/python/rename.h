// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

template <class T>
T rename_dims(T &self, const std::map<std::string, std::string> &name_dict) {
  auto out(self);
  for (const auto &[from, to] : name_dict)
    out.rename(Dim{from}, Dim{to});
  return out;
}
