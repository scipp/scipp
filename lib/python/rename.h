// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

template <class T>
T rename_dims(T &self, const std::map<std::string, std::string> &name_dict) {
  std::vector<std::pair<Dim, Dim>> names;
  names.reserve(name_dict.size());
  for (const auto &[from, to] : name_dict)
    names.emplace_back(Dim{from}, Dim{to});
  return self.rename_dims(names);
}
