// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_AXIS_H
#define SCIPP_CORE_AXIS_H

#include <boost/iterator/transform_iterator.hpp>

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

class DatasetAxisConstView;
class DatasetAxisView;

class DatasetAxis {
public:
  using const_view_type = DatasetAxisConstView;
  using view_type = DatasetAxisView;
  using unaligned_type = std::unordered_map<std::string, Variable>;
  using unaligned_const_view_type = UnalignedConstView;
  using unaligned_view_type = UnalignedView;

  explicit DatasetAxis(Variable data) : m_data(std::move(data)) {}

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

  /// Return true if the data array contains data values.
  bool hasData() const noexcept { return static_cast<bool>(m_data); }
  /// Return untyped const view for data (values and optional variances).
  VariableConstView data() const {
    if (hasData())
      return m_data;
    throw except::SparseDataError("No data in item.");
  }

  // TODO only return empty if there is unaligned? just throw?
  // actually need to look at coords in case of unaligned data to determine dims
  Dimensions dims() const { return hasData() ? data().dims() : Dimensions(); }
  DType dtype() const { return data().dtype(); }
  units::Unit unit() const { return data().unit(); }

  /// Return true if the data array contains data variances.
  bool hasVariances() const { return data().hasVariances(); }

  /// Return typed const view for data values.
  template <class T> auto values() const { return data().template values<T>(); }

  /// Return typed const view for data variances.
  template <class T> auto variances() const {
    return data().template variances<T>();
  }

  /// Return untyped view for data (values and optional variances).
  VariableView data() {
    if (static_cast<Derived *>(this)->m_data)
      return static_cast<Derived *>(this)->m_data;
    throw except::SparseDataError("No data in item.");
  }

  void setUnit(const units::Unit unit) { data().setUnit(unit); }

  /// Return typed view for data values.
  template <class T> auto values() { return data().template values<T>(); }

  /// Return typed view for data variances.
  template <class T> auto variances() { return data().template variances<T>(); }

  // Axis &operator+=(const AxisConstView<T> &other);
  // Axis &operator-=(const AxisConstView<T> &other);
  // Axis &operator*=(const AxisConstView<T> &other);
  // Axis &operator/=(const AxisConstView<T> &other);

private:
  Variable m_data;
  unaligned_type m_unaligned;
};

class DatasetAxisConstView {
public:
  DatasetAxisConstView(const DatasetAxis &axis) : m_data(axis.data()) {
    auto u = axis.unaligned();
    m_unaligned.insert(u.begin(), u.end());
  }
  // Implicit conversion from VariableConstView useful for operators.
  DatasetAxisConstView(const VariableConstView &data) : m_data(data) {}

  DatasetAxisConstView slice(const Slice slice) const { return *this; }

private:
  VariableView m_data;
  std::unordered_map<std::string, VariableView> m_unaligned;
};

class DatasetAxisView : public DatasetAxisConstView {
public:
  DatasetAxisView(DatasetAxis &data) : DatasetAxisConstView(data) {}

  DatasetAxisView slice(const Slice slice) const { return *this; }
};


// DataArrayView slice(const Slice slice1) const;
//
// DataArrayView assign(const DataArrayConstView &other) const;
// DataArrayView assign(const Variable &other) const;
// DataArrayView assign(const VariableConstView &other) const;

#if 0
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
#endif

} // namespace next
} // namespace scipp::core
#endif // SCIPP_CORE_AXIS_H
