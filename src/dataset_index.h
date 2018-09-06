/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DATASET_INDEX_H
#define DATASET_INDEX_H

#include <unordered_map>

#include "dataset.h"

template <class Tag> class DatasetIndex {
public:
  DatasetIndex(const Dataset &dataset) {
    const auto &axis = dataset.get<const Tag>();
    gsl::index current = 0;
    for (auto item : axis)
      m_index[item] = current++;
    if (axis.size() != m_index.size())
      throw std::runtime_error("Axis contains duplicate labels. Cannot use it "
                               "to index into the data.");
  }

  gsl::index operator[](const typename Tag::type &key) const {
    return m_index.at(key);
  }

private:
  std::unordered_map<typename Tag::type, gsl::index> m_index;
};

#endif // DATASET_INDEX_H
