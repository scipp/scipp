// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_DATA_INTERFACE_H
#define SCIPP_CORE_DATA_INTERFACE_H

#include "scipp/core/variable.h"

namespace scipp::core {

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

  Dimensions dims() const { return hasData() ? data().dims() : Dimensions(); }
  DType dtype() const { return data().dtype(); }
  units::Unit unit() const { return data().unit(); }

  /// Return true if the data array contains data variances.
  bool hasVariances() const { return data().hasVariances(); }

  /// Return typed const view for data values.
  template <class T> auto values() const { return data().values<T>(); }

  /// Return typed const view for data variances.
  template <class T> auto variances() const { return data().variances<T>(); }
};

template <class Derived>
struct DataViewInterface : DataConstInterface<Derived> {
  /// Return untyped view for data (values and optional variances).
  VariableView data() const {
    if (hasData())
      return static_cast<const Derived *>(this)->.m_data;
    throw except::SparseDataError("No data in item.");
  }

  void setUnit(const units::Unit unit) const { data().setUnit(unit); }

  /// Return typed view for data values.
  template <class T> auto values() const { return data().values<T>(); }

  /// Return typed view for data variances.
  template <class T> auto variances() const { return data().variances<T>(); }
};

template <class Derived> struct DataInterface : DataConstInterface<Derived> {
  /// Return untyped view for data (values and optional variances).
  VariableView data() {
    if (hasData())
      return static_cast<Derived *>(this)->.m_data;
    throw except::SparseDataError("No data in item.");
  }

  void setUnit(const units::Unit unit) { data().setUnit(unit); }

  /// Return typed view for data values.
  template <class T> auto values() { return data().values<T>(); }

  /// Return typed view for data variances.
  template <class T> auto variances() { return data().variances<T>(); }
};

} // namespace scipp::core
#endif // SCIPP_CORE_DATA_INTERFACE_H
