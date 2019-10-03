// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_PYTHON_RENAME_H
#define SCIPP_PYTHON_RENAME_H

#include "scipp/core/dataset.h"

template <class T>
void rename_dims(T &self, const std::map<Dim, Dim> &name_dict) {
  for (const auto & [ from, to ] : name_dict)
    self.rename(from, to);
}

#endif // SCIPP_PYTHON_RENAME_H
