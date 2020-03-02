// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_AXIS_H
#define SCIPP_CORE_AXIS_H

#include "scipp-core_export.h"
#include "scipp/core/axis_forward.h"
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

namespace axis_detail {
template <class T> struct const_view;
template <> struct const_view<Variable> {
  using type = typename Variable::const_view_type;
};
template <> struct const_view<axis::dataset_unaligned_type> {
  using type = UnalignedConstView;
};
template <class T> struct view;
template <> struct view<Variable> {
  using type = typename Variable::view_type;
};
template <> struct view<axis::dataset_unaligned_type> {
  using type = UnalignedView;
};
template <class T> using const_view_t = typename const_view<T>::type;
template <class T> using view_t = typename view<T>::type;
} // namespace axis_detail

template <class Id, class UnalignedType> class Axis {
public:
  using const_view_type = AxisConstView<Axis>;
  using view_type = AxisView<Axis>;
  using unaligned_type = UnalignedType;
  using unaligned_const_view_type = axis_detail::const_view_t<unaligned_type>;
  using unaligned_view_type = axis_detail::view_t<unaligned_type>;

  Axis() = default;
  explicit Axis(Variable data) : m_data(std::move(data)) {}
  Axis(Variable data, unaligned_type unaligned)
      : m_data(std::move(data)), m_unaligned(std::move(unaligned)) {}
  explicit Axis(const const_view_type &data);

  unaligned_const_view_type unaligned() const;
  unaligned_view_type unaligned();

  template <typename T = AxisId::Dataset>
  std::enable_if_t<std::is_same_v<T, AxisId::Dataset>, DataArrayAxisView>
  operator[](const std::string &name) {
    return m_unaligned.count(name)
               ? DataArrayAxisView(m_data, m_unaligned.at(name))
               : DataArrayAxisView(m_data);
  }
  template <typename T = AxisId::Dataset>
  std::enable_if_t<std::is_same_v<T, AxisId::Dataset>, DataArrayAxisConstView>
  operator[](const std::string &name) const {
    return m_unaligned.count(name)
               ? DataArrayAxisConstView(VariableView(m_data),
                                        VariableView(m_unaligned.at(name)))
               : DataArrayAxisConstView(m_data);
  }

  /// Return true if the axis contains data values.
  bool hasData() const noexcept { return static_cast<bool>(m_data); }
  /// Return true if the axis contains unaligned data values.
  bool hasUnaligned() const noexcept {
    if constexpr (std::is_same_v<Id, AxisId::DataArray>)
      return static_cast<bool>(m_unaligned);
    else
      return m_unaligned.empty();
  }
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

  Axis &operator+=(const const_view_type &other);
  Axis &operator-=(const const_view_type &other);
  Axis &operator*=(const const_view_type &other);
  Axis &operator/=(const const_view_type &other);

  void rename(const Dim from, const Dim to);

  static DatasetAxis to_DatasetAxis(DataArrayAxis &&axis,
                                    const std::string &key) {
    if constexpr (std::is_same_v<Id, AxisId::DataArray>) {
      DatasetAxis out(std::move(axis.m_data));
      if (axis.m_unaligned)
        out.unaligned().set(key, std::move(axis.m_unaligned));
      return out;
    } else {
      static_cast<void>(axis);
      static_cast<void>(key);
      throw std::logic_error("");
    }
  }

private:
  friend class AxisConstView<Axis>;
  Variable m_data;
  unaligned_type m_unaligned;
};

template <class Axis> class AxisConstView {
public:
  using value_type = Axis;
  using unaligned_type = typename value_type::unaligned_type;
  using unaligned_const_view_type =
      typename value_type::unaligned_const_view_type;
  using unaligned_view_type = typename value_type::unaligned_view_type;

  AxisConstView(const value_type &axis);
  AxisConstView(VariableView &&data, unaligned_view_type &&unaligned);
  AxisConstView(VariableView &&data);
  AxisConstView(VariableConstView &&data);

  const unaligned_const_view_type &unaligned() const noexcept;

  /// Return true if the axis contains data values.
  bool hasData() const noexcept { return static_cast<bool>(m_data); }
  /// Return true if the axis contains unaligned data values.
  bool hasUnaligned() const noexcept {
    if constexpr (std::is_same_v<Axis, DataArrayAxis>)
      return static_cast<bool>(m_unaligned);
    else
      return m_unaligned.empty();
  }
  VariableConstView data() const;

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

  AxisConstView slice(const Slice s) const;

protected:
  // Note: Not const views to avoid duplicate view creation
  VariableView m_data;
  unaligned_view_type m_unaligned;

private:
  unaligned_view_type
  make_unaligned(unaligned_const_view_type const_view) const;
  unaligned_view_type make_empty_unaligned() const;
};

template <class Axis> class AxisView : public AxisConstView<Axis> {
public:
  using value_type = Axis;
  using unaligned_type = typename value_type::unaligned_type;
  using unaligned_view_type = typename value_type::unaligned_view_type;

  AxisView(VariableView data) : AxisConstView<Axis>(std::move(data)) {}
  AxisView(Axis &data)
      : AxisConstView<Axis>(data.hasData() ? data.data() : VariableView{},
                            data.hasUnaligned() ? data.unaligned()
                                                : unaligned_view_type{}) {}
  // TODO this should be private
  AxisView(AxisConstView<Axis> &&base) : AxisConstView<Axis>(std::move(base)) {}
  AxisView(VariableView &&data, unaligned_view_type &&unaligned)
      : AxisConstView<Axis>(std::move(data), std::move(unaligned)) {}

  const unaligned_view_type &unaligned() const noexcept;
  VariableView data() const;

  void setUnit(const units::Unit unit) const { data().setUnit(unit); }

  /// Return typed view for data values.
  template <class T> auto values() const { return data().template values<T>(); }

  /// Return typed view for data variances.
  template <class T> auto variances() const {
    return data().template variances<T>();
  }

  AxisView slice(const Slice s) const;

  AxisView operator+=(const VariableConstView &other) const;
  AxisView operator-=(const VariableConstView &other) const;
  AxisView operator*=(const VariableConstView &other) const;
  AxisView operator/=(const VariableConstView &other) const;
  AxisView operator+=(const AxisConstView<Axis> &other) const;
  AxisView operator-=(const AxisConstView<Axis> &other) const;
  AxisView operator*=(const AxisConstView<Axis> &other) const;
  AxisView operator/=(const AxisConstView<Axis> &other) const;
};

SCIPP_CORE_EXPORT bool operator==(const DataArrayAxisConstView &a,
                                  const DataArrayAxisConstView &b);
SCIPP_CORE_EXPORT bool operator!=(const DataArrayAxisConstView &a,
                                  const DataArrayAxisConstView &b);
SCIPP_CORE_EXPORT bool operator==(const DatasetAxisConstView &a,
                                  const DatasetAxisConstView &b);
SCIPP_CORE_EXPORT bool operator!=(const DatasetAxisConstView &a,
                                  const DatasetAxisConstView &b);

SCIPP_CORE_EXPORT bool operator==(const DataArrayAxisConstView &a,
                                  const VariableConstView &b);
SCIPP_CORE_EXPORT bool operator!=(const DataArrayAxisConstView &a,
                                  const VariableConstView &b);
SCIPP_CORE_EXPORT bool operator==(const VariableConstView &a,
                                  const DataArrayAxisConstView &b);
SCIPP_CORE_EXPORT bool operator!=(const VariableConstView &a,
                                  const DataArrayAxisConstView &b);

SCIPP_CORE_EXPORT bool operator==(const DatasetAxisConstView &a,
                                  const VariableConstView &b);
SCIPP_CORE_EXPORT bool operator!=(const DatasetAxisConstView &a,
                                  const VariableConstView &b);
SCIPP_CORE_EXPORT bool operator==(const VariableConstView &a,
                                  const DatasetAxisConstView &b);
SCIPP_CORE_EXPORT bool operator!=(const VariableConstView &a,
                                  const DatasetAxisConstView &b);

SCIPP_CORE_EXPORT DataArrayAxis concatenate(const DataArrayAxisConstView &a1,
                                            const DataArrayAxisConstView &a2,
                                            const Dim dim);
SCIPP_CORE_EXPORT DatasetAxis concatenate(const DatasetAxisConstView &a1,
                                          const DatasetAxisConstView &a2,
                                          const Dim dim);
SCIPP_CORE_EXPORT DataArrayAxis resize(const DataArrayAxisConstView &var,
                                       const Dim dim, const scipp::index size);
SCIPP_CORE_EXPORT DatasetAxis resize(const DatasetAxisConstView &var,
                                     const Dim dim, const scipp::index size);

[[nodiscard]] SCIPP_CORE_EXPORT DatasetAxis
flatten(const DatasetAxisConstView &var, const Dim dim);

SCIPP_CORE_EXPORT DataArrayAxis copy(const DataArrayAxisConstView &axis);
SCIPP_CORE_EXPORT DatasetAxis copy(const DatasetAxisConstView &axis);

} // namespace scipp::core

#endif // SCIPP_CORE_AXIS_H
