// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_AXIS_H
#define SCIPP_CORE_AXIS_H

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/data_interface.h"
#include "scipp/core/map.h"
#include "scipp/core/variable.h"

namespace scipp::core {
namespace next {

// d.coords
// d['a'].coords # align, thus the same
// d['a'].coords['x'] # Axis, i.e., dense and unaligned
// d['a'].coords['x'].data # dense variable
// d['a'].unaligned.coords
//
// d.masks
// d['a'].masks # would like to support different
// d['a'].unaligned.masks
//
// d.attrs
// d['a'].attrs
// d['a'].unaligned.attrs

template <class T> class AxisConstView;
template <class T> class AxisView;

template <class T> class Axis : public DataInterface<Axis<T>> {
public:
  using const_view_type = AxisConstView<T>;
  using view_type = AxisView<T>;
  using unaligned_type =
      std::conditional_t<std::is_same_v<T, AxisId::Dataset>,
                         std::unordered_map<std::string, Variable>, Variable>;
  using unaligned_const_view_type =
      std::conditional_t<std::is_same_v<T, AxisId::Dataset>, UnalignedConstView,
                         VariableConstView>;
  using unaligned_view_type =
      std::conditional_t<std::is_same_v<T, AxisId::Dataset>, UnalignedView,
                         VariableView>;

  explicit Axis(Variable data) : m_data(std::move(data)) {}


  auto unaligned() const noexcept {
    std::unordered_map<std::string, std::pair<const Variable *, Variable *>>
        items;
    for (const auto &[key, value] : m_unaligned)
      items.emplace(key, std::pair{&value, nullptr});
    return unaligned_const_view_type{std::move(items)};
  }
  auto unaligned() noexcept {
    std::unordered_map<std::string, std::pair<const Variable *, Variable *>>
        items;
    for (auto &&[key, value] : m_unaligned)
      items.emplace(key, std::pair{&value, &value});

    return unaligned_view_type{UnalignedAccess(this, &m_unaligned),
                               std::move(items)};
  }

  // Axis &operator+=(const AxisConstView<T> &other);
  // Axis &operator-=(const AxisConstView<T> &other);
  // Axis &operator*=(const AxisConstView<T> &other);
  // Axis &operator/=(const AxisConstView<T> &other);

  Variable m_data;
  unaligned_type m_unaligned;
};

template <class T> class DataViewInterface;

template <class T>
class AxisConstView : public DataConstInterface<AxisConstView<T>> {
public:
  AxisConstView(const Axis<T> &axis) : m_data(axis.data()) {
    auto u = axis.unaligned();
    m_unaligned.insert(u.begin(), u.end());
  }
  // Implicit conversion from VariableConstView useful for operators.
  AxisConstView(const VariableConstView &data) : m_data(data) {}

  AxisConstView slice(const Slice slice) const { return *this; }

  // Need two different interfaces:
  // For coord of dataset
  // const UnalignedConstView unaligned() const noexcept { return m_unaligned; }
  // const UnalignedView unaligned() noexcept { return m_unaligned; }
  // For coord of data array or data of dataset item or data array
  // const VarialeConstView unaligned() const noexcept { return m_unaligned; }
  // const VarialeView unaligned() noexcept { return m_unaligned; }

  VariableView m_data;
  std::unordered_map<std::string, VariableView> m_unaligned;
};

template <class T>
class AxisView : public AxisConstView<T>,
                 public DataViewInterface<AxisView<T>> {
public:
  AxisView(Axis<T> &data) : AxisConstView<T>(data) {}

  AxisView slice(const Slice slice) const { return *this; }

  using DataViewInterface<AxisView<T>>::data;

  // AxisView operator+=(const AxisConstView &other) const;
  // AxisView operator-=(const AxisConstView &other) const;
  // AxisView operator*=(const AxisConstView &other) const;
  // AxisView operator/=(const AxisConstView &other) const;
};

struct Dataset {
  std::unordered_map<std::string, Variable> items;
  std::unordered_map<Dim, Axis<AxisId::Dataset>> coords;
};

// DataArrayView slice(const Slice slice1) const;
//
// DataArrayView assign(const DataArrayConstView &other) const;
// DataArrayView assign(const Variable &other) const;
// DataArrayView assign(const VariableConstView &other) const;

void CoordAccess::set(const Dim &key, Variable var) const {
  // No restrictions on dimension of unaligned? Do we need to ensure
  // unaligned does not depend on dimension of parent?
  // TODO enforce same unit?
  m_coords->emplace(key, std::move(var));
}

void CoordAccess::erase(const Dim &key) const {
  m_coords->erase(m_coords->find(key));
}

auto coords(const Dataset &dataset) {
  typename CoordsConstView::holder_type items;
  for (const auto &[key, value] : dataset.coords)
    items.emplace(key, std::pair{&value, nullptr});
  return CoordsConstView{std::move(items)};
}
auto coords(Dataset &dataset) {
  typename CoordsView::holder_type items;
  for (auto &&[key, value] : dataset.coords)
    items.emplace(key, std::pair{&value, &value});

  return CoordsView{CoordAccess(&dataset, &dataset.coords), std::move(items)};
}

} // namespace next
} // namespace scipp::core
#endif // SCIPP_CORE_AXIS_H
