// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_MAP_H
#define SCIPP_CORE_MAP_H

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/except.h"
#include "scipp/core/map_interface.h"
#include "scipp/core/slice.h"
#include "scipp/core/variable.h"
#include "scipp/units/unit.h"

namespace scipp::core {
namespace next {

class Dataset;

template <class T> class Axis;
namespace AxisId {
class Dataset;
class Datarray;
} // namespace AxisId

namespace ViewId {
class Attrs;
class Coords;
class Masks;
class Unaligned;
} // namespace ViewId
template <class Id, class Key, class Value> class ConstView;
template <class Base, class ParentAccess> class MutableView;

// Helper class to add or erase unaligned items.
// Includes sanity checks such as dims (what else? unit? dtype?)
class UnalignedAccess {
public:
  UnalignedAccess(const Axis<AxisId::Dataset> *parent,
                  std::unordered_map<std::string, Variable> *unaligned)
      : m_parent(parent), m_unaligned(unaligned) {}

  void set(const std::string &key, Variable var) const {
    // No restrictions on dimension of unaligned? Do we need to ensure
    // unaligned does not depend on dimension of parent?
    // TODO enforce same unit?
    m_unaligned->emplace(key, std::move(var));
  }

  void erase(const std::string &key) const {
    m_unaligned->erase(m_unaligned->find(key));
  }

private:
  const Axis<AxisId::Dataset> *m_parent;
  std::unordered_map<std::string, Variable> *m_unaligned;
};

class CoordAccess {
public:
  CoordAccess(const Dataset *parent,
              std::unordered_map<Dim, Axis<AxisId::Dataset>> *coords)
      : m_parent(parent), m_coords(coords) {}

  void set(const Dim &key, Variable var) const;

  void erase(const Dim &key) const;

private:
  const Dataset *m_parent;
  std::unordered_map<Dim, Axis<AxisId::Dataset>> *m_coords;
};

// TODO need two different ones, also for DataArray!
/// View for accessing coordinates of const Dataset and DataArrayConstView.
using CoordsConstView = ConstView<ViewId::Coords, Dim, Axis<AxisId::Dataset>>;
/// View for accessing coordinates of Dataset and DataArrayView.
using CoordsView = MutableView<CoordsConstView, CoordAccess>;
/// View for accessing attributes of const Dataset and DataArrayConstView.
using AttrsConstView = ConstView<ViewId::Attrs, std::string, Variable>;
/// View for accessing attributes of Dataset and DataArrayView.
// using AttrsView = MutableView<AttrsConstView>;
/// View for accessing masks of const Dataset and DataArrayConstView
using MasksConstView = ConstView<ViewId::Masks, std::string, Variable>;
/// View for accessing masks of Dataset and DataArrayView
// using MasksView = MutableView<MasksConstView>;

/// View for accessing unaligned items of a const dataset axis
using UnalignedConstView = ConstView<ViewId::Unaligned, std::string, Variable>;
/// View for accessing unaligned items of a dataset axis
using UnalignedView = MutableView<UnalignedConstView, UnalignedAccess>;

/// Return the dimension for given coord.
/// @param var Coordinate variable
/// @param key Key of the coordinate in a coord dict
///
/// For dimension-coords, this is the same as the key, for non-dimension-coords
/// (labels) we adopt the convention that they are "label" their inner
/// dimension.
template <class T, class Key> Dim dim_of_coord(const T &var, const Key &key) {
  if constexpr (std::is_same_v<Key, Dim>) {
    const bool is_dimension_coord = var.dims().contains(key);
    return is_dimension_coord ? key : var.dims().inner();
  } else
    return var.dims().inner();
}

// TODO Can we also use this for DatasetConstView?
// No, but at least reuse the interfaces?
/// Common functionality for other const-view classes.
template <class Id, class Key, class Value>
class ConstView
    : public MapConstInterface<ConstView<Id, Key, Value>, Key, Value> {
private:
  struct make_item {
    const ConstView *view;
    template <class T> auto operator()(const T &item) const {
      return std::pair<Key, typename ConstView::mapped_type::const_view_type>(
          item.first, detail::makeSlice(*item.second.first, view->slices()));
    }
  };

public:
  using key_type = Key;
  using mapped_type = Value;
  using holder_type =
      std::unordered_map<key_type,
                         std::pair<const mapped_type *, mapped_type *>>;

  ConstView(holder_type &&items, const detail::slice_list &slices = {})
      : m_items(std::move(items)), m_slices(slices) {
    // TODO This is very similar to the code in makeViewItems(), provided that
    // we can give a good definion of the `dims` argument (roughly the space
    // spanned by all coords, excluding the dimensions that are sliced away).
    // Remove any items for a non-range sliced dimension. Identified via the
    // item in case of coords, or via the inner dimension in case of labels and
    // attributes.
#if 0
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
                return _.second.first->dims().contains(_.first);
              };
              return is_dimension_coord(it2)
                         ? it2.first == slice.dim()
                         : (!it2.second.first->dims().empty() &&
                            (it2.second.first->dims().inner() == slice.dim()));
            } else {
              return !it2.second.first->dims().empty() &&
                     (it2.second.first->dims().inner() == slice.dim());
            }
          };
          if (erase(*it))
            it = m_items.erase(it);
          else
            ++it;
        }
      }
    }
#endif
  }

  ConstView slice(const Slice slice1) const {
    auto slices = m_slices;
    if constexpr (std::is_same_v<Key, Dim>) {
      const auto &coord = *m_items.at(slice1.dim()).first;
      slices.emplace_back(slice1, coord.dims()[slice1.dim()]);
    } else {
      throw std::runtime_error("TODO");
    }
    auto items = m_items;
    return ConstView(std::move(items), slices);
  }

#if 0
  bool operator==(const ConstView &other) const {
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
  bool operator!=(const ConstView &other) const { return !operator==(other); }
#endif

protected:
  friend class MapConstInterface<ConstView<Id, Key, Value>, Key, Value>;
  holder_type m_items;
  detail::slice_list m_slices;
};

/// Common functionality for other view classes.
template <class Base, class ParentAccess>
class MutableView
    : public Base,
      public MapInterface<MutableView<Base, ParentAccess>,
                          typename Base::key_type, typename Base::mapped_type> {
private:
  MutableView(ParentAccess &&parent, Base &&base)
      : Base(std::move(base)), m_parent(std::move(parent)) {}

  ParentAccess m_parent;

public:
  using MapInterface<MutableView<Base, ParentAccess>, typename Base::key_type,
                     typename Base::mapped_type>::operator[];

  MutableView(ParentAccess &&parent, typename Base::holder_type &&items,
              const detail::slice_list &slices = {})
      : Base(std::move(items), slices), m_parent(std::move(parent)) {}

  MutableView slice(const Slice slice1) const {
    // parent = nullptr since adding coords via slice is not supported.
    return MutableView(m_parent, Base::slice(slice1));
  }

  template <class VarOrView>
  void set(const typename Base::key_type key, VarOrView var) const {
    m_parent.set(key, std::move(var));
  }

  void erase(const typename Base::key_type key) const { m_parent.erase(key); }
};

} // namespace next
} // namespace scipp::core

#endif // SCIPP_CORE_MAP_H
