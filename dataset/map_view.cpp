// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/map_view.h"

namespace scipp::dataset {

template <class Id, class Key, class Value>
ConstView<Id, Key, Value>::ConstView(holder_type &&items,
                                     const detail::slice_list &slices)
    : m_items(std::move(items)), m_slices(slices) {
  // TODO This is very similar to the code in makeViewItems(), provided that
  // we can give a good definion of the `dims` argument (roughly the space
  // spanned by all coords, excluding the dimensions that are sliced away).
  // Remove any items for a non-range sliced dimension. Identified via the
  // item in case of coords, or via the inner dimension in case of labels and
  // attributes.
  for (const auto &s : m_slices) {
    const auto slice = s.first;
    if (!slice.isRange()) { // The slice represents a point not a range.
                            // Dimension removed.
      for (auto it = m_items.begin(); it != m_items.end();) {
        auto erase = [slice](const auto &it2) {
          if constexpr (std::is_same_v<Key, Dim>) {
            // Remove dimension-coords for given dim, or non-dimension coords
            // if their inner dim is the given dim.
            constexpr auto is_dimension_coord = [](const auto &_) {
              return !contains_events(_.second) &&
                     _.second.dims().contains(_.first);
            };
            return is_dimension_coord(it2)
                       ? it2.first == slice.dim()
                       : (!it2.second.dims().empty() &&
                          !contains_events(it2.second) &&
                          (it2.second.dims().inner() == slice.dim()));
          } else {
            return !it2.second.dims().empty() && !contains_events(it2.second) &&
                   (it2.second.dims().inner() == slice.dim());
          }
        };
        if (erase(*it))
          it = m_items.erase(it);
        else
          ++it;
      }
    }
  }
}

template <class Id, class Key, class Value>
bool ConstView<Id, Key, Value>::operator==(const ConstView &other) const {
  if (size() != other.size())
    return false;
  for (const auto &[name, data] : *this) {
    try {
      if (data != other[name])
        return false;
    } catch (except::NotFoundError &) {
      return false;
    }
  }
  return true;
}

template <class Id, class Key, class Value>
bool ConstView<Id, Key, Value>::operator!=(const ConstView &other) const {
  return !operator==(other);
}

template <class Base, class Access>
MutableView<Base, Access>::MutableView(const Access &access,
                                       typename Base::holder_type &&items,
                                       const detail::slice_list &slices)
    : Base(std::move(items), slices), m_access(access) {}

template <class Base, class Access>
MutableView<Base, Access>::MutableView(const Access &access, Base &&base)
    : Base(std::move(base)), m_access(access) {}

template class ConstView<ViewId::Coords, Dim, variable::Variable>;
template class MutableView<CoordsConstView, CoordAccess>;
template class ConstView<ViewId::Attrs, std::string, variable::Variable>;
template class MutableView<AttrsConstView, AttrAccess>;
template class ConstView<ViewId::Masks, std::string, variable::Variable>;
template class MutableView<MasksConstView, MaskAccess>;

} // namespace scipp::dataset
