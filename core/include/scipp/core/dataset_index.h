// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DATASET_INDEX_H
#define DATASET_INDEX_H

#include <unordered_map>

#include "scipp/core/dataset.h"

namespace scipp::core {

template <class Tag> class DatasetIndex {
public:
  DatasetIndex(const Dataset &dataset) {
    const auto &axis = dataset.get(Tag{});
    scipp::index current = 0;
    for (auto item : axis)
      m_index[item] = current++;
    if (scipp::size(axis) != scipp::size(m_index))
      throw std::runtime_error("Axis contains duplicate labels. Cannot use it "
                               "to index into the data.");
  }

  scipp::index operator[](const typename Tag::type &key) const {
    return m_index.at(key);
  }

private:
  std::unordered_map<typename Tag::type, scipp::index> m_index;
};

} // namespace scipp::core

#endif // DATASET_INDEX_H
