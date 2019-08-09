// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <ostream>

#include "scipp/core/dataset.h"
#include "scipp/core/except.h"

namespace scipp::core {

DataArray::DataArray(Variable data, std::map<Dim, Variable> coords,
                     std::map<std::string, Variable> labels) {
  m_holder.setData("", std::move(data));
  for (auto & [ dim, c ] : coords)
    m_holder.setCoord(dim, std::move(c));
  for (auto & [ name, l ] : labels)
    m_holder.setLabels(name, std::move(l));
}

} // namespace scipp::core
