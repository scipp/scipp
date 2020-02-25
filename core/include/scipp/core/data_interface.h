// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_DATA_INTERFACE_H
#define SCIPP_CORE_DATA_INTERFACE_H

#include "scipp/core/except.h"
#include "scipp/core/variable.h"

namespace scipp::core {
namespace next {

template <class Derived> struct DataConstInterface {
  /// Return true if the data array contains data values.
  bool hasData() const {
    return static_cast<bool>(static_cast<const Derived *>(this)->m_data);
  }
  /// Return untyped const view for data (values and optional variances).
  VariableConstView data() const {
    if (hasData())
      return static_cast<const Derived *>(this)->m_data;
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
};

template <class Derived> struct DataViewInterface {
  /// Return untyped view for data (values and optional variances).
  VariableView data() const {
    if (static_cast<const Derived *>(this)->m_data)
      return static_cast<const Derived *>(this)->m_data;
    throw except::SparseDataError("No data in item.");
  }

  void setUnit(const units::Unit unit) const { data().setUnit(unit); }

  /// Return typed view for data values.
  template <class T> auto values() const { return data().template values<T>(); }

  /// Return typed view for data variances.
  template <class T> auto variances() const {
    return data().template variances<T>();
  }
};

template <class Derived>
struct DataInterface : public DataConstInterface<Derived> {
  using DataConstInterface<Derived>::data;
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
};

} // namespace next
} // namespace scipp::core
#endif // SCIPP_CORE_DATA_INTERFACE_H
