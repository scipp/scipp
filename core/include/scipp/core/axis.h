// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_AXIS_H
#define SCIPP_CORE_AXIS_H

#include "scipp-core_export.h"
#include "scipp/core/except.h"
#include "scipp/core/variable.h"
#include "scipp/core/view_decl.h"
#include "scipp/units/dim.h"

namespace scipp::except {
struct SCIPP_CORE_EXPORT UnalignedError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};
} // namespace scipp::except

namespace scipp::core {

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

class SCIPP_CORE_EXPORT DatasetAxis {
public:
  using const_view_type = DatasetAxisConstView;
  using view_type = DatasetAxisView;
  using unaligned_type = std::unordered_map<std::string, Variable>;
  using unaligned_const_view_type = UnalignedConstView;
  using unaligned_view_type = UnalignedView;

  DatasetAxis() = default;
  explicit DatasetAxis(Variable data) : m_data(std::move(data)) {}
  explicit DatasetAxis(const DatasetAxisConstView &data);

  UnalignedConstView unaligned() const;
  UnalignedView unaligned();

  /// Return true if the data array contains data values.
  bool hasData() const noexcept { return static_cast<bool>(m_data); }
  /// Return untyped const view for data (values and optional variances).
  VariableConstView data() const {
    if (hasData())
      return m_data;
    throw except::SparseDataError("No data in item.");
  }
  /// Return untyped view for data (values and optional variances).
  VariableView data() {
    if (hasData())
      return m_data;
    throw except::SparseDataError("No data in item.");
  }

  // TODO only return empty if there is unaligned? just throw?
  // actually need to look at coords in case of unaligned data to determine dims
  Dimensions dims() const { return hasData() ? m_data.dims() : Dimensions(); }
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

  void setUnit(const units::Unit unit) { data().setUnit(unit); }

  /// Return typed view for data values.
  template <class T> auto values() { return data().template values<T>(); }

  /// Return typed view for data variances.
  template <class T> auto variances() { return data().template variances<T>(); }

  DatasetAxis &operator+=(const DatasetAxisConstView &other);
  DatasetAxis &operator-=(const DatasetAxisConstView &other);
  DatasetAxis &operator*=(const DatasetAxisConstView &other);
  DatasetAxis &operator/=(const DatasetAxisConstView &other);

  void rename(const Dim from, const Dim to);

private:
  friend class DatasetAxisConstView;
  Variable m_data;
  unaligned_type m_unaligned;
};

class SCIPP_CORE_EXPORT DatasetAxisConstView {
public:
  using value_type = DatasetAxis;

  DatasetAxisConstView(const DatasetAxis &axis)
      : m_data(axis.m_data), m_unaligned(UnalignedAccess{}, axis.unaligned()) {}
  /// Constructor used by DatasetAxisView
  DatasetAxisConstView(VariableView &&data, UnalignedView &&view)
      : m_data(std::move(data)), m_unaligned(std::move(view)) {}
  // Implicit conversion from VariableConstView useful for operators.
  DatasetAxisConstView(VariableConstView &&data)
      : m_data(std::move(data)),
        m_unaligned(UnalignedAccess{},
                    typename UnalignedConstView::holder_type{}) {}

  const UnalignedConstView &unaligned() const noexcept;

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

  DatasetAxisConstView slice(const Slice) const { return *this; }

protected:
  VariableView m_data;
  UnalignedView m_unaligned;
};

class SCIPP_CORE_EXPORT DatasetAxisView : public DatasetAxisConstView {
public:
  DatasetAxisView(DatasetAxis &data)
      : DatasetAxisConstView(data.data(), data.unaligned()) {}

  const UnalignedView &unaligned() const noexcept;

  /// Return untyped view for data (values and optional variances).
  VariableView data() const {
    if (hasData())
      return m_data;
    throw except::SparseDataError("No data in item.");
  }

  void setUnit(const units::Unit unit) const { data().setUnit(unit); }

  /// Return typed view for data values.
  template <class T> auto values() const { return data().template values<T>(); }

  /// Return typed view for data variances.
  template <class T> auto variances() const {
    return data().template variances<T>();
  }

  DatasetAxisView slice(const Slice) const { return *this; }

  DatasetAxisView operator+=(const VariableConstView &other) const;
  DatasetAxisView operator-=(const VariableConstView &other) const;
  DatasetAxisView operator*=(const VariableConstView &other) const;
  DatasetAxisView operator/=(const VariableConstView &other) const;
  DatasetAxisView operator+=(const DatasetAxisConstView &other) const;
  DatasetAxisView operator-=(const DatasetAxisConstView &other) const;
  DatasetAxisView operator*=(const DatasetAxisConstView &other) const;
  DatasetAxisView operator/=(const DatasetAxisConstView &other) const;
};

SCIPP_CORE_EXPORT bool operator==(const DatasetAxisConstView &,
                                  const DatasetAxisConstView &);
SCIPP_CORE_EXPORT bool operator!=(const DatasetAxisConstView &a,
                                  const DatasetAxisConstView &b);
SCIPP_CORE_EXPORT bool operator==(const DatasetAxisConstView &,
                                  const VariableConstView &);
SCIPP_CORE_EXPORT bool operator!=(const DatasetAxisConstView &a,
                                  const VariableConstView &b);
SCIPP_CORE_EXPORT bool operator==(const VariableConstView &,
                                  const DatasetAxisConstView &);
SCIPP_CORE_EXPORT bool operator!=(const VariableConstView &a,
                                  const DatasetAxisConstView &b);

SCIPP_CORE_EXPORT DatasetAxis concatenate(const DatasetAxisConstView &a1,
                                          const DatasetAxisConstView &a2,
                                          const Dim dim);
SCIPP_CORE_EXPORT DatasetAxis resize(const DatasetAxisConstView &var,
                                     const Dim dim, const scipp::index size);

[[nodiscard]] SCIPP_CORE_EXPORT DatasetAxis
flatten(const DatasetAxisConstView &var, const Dim dim);

SCIPP_CORE_EXPORT DatasetAxis copy(const DatasetAxisConstView &axis);

} // namespace scipp::core

#endif // SCIPP_CORE_AXIS_H
