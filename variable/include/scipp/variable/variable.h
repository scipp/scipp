// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <Eigen/Dense>

#include "scipp-variable_export.h"
#include "scipp/common/index.h"
#include "scipp/common/span.h"
#include "scipp/units/unit.h"

#include "scipp/core/dimensions.h"
#include "scipp/core/dtype.h"
#include "scipp/core/element_array.h"
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"
#include "scipp/core/slice.h"
#include "scipp/core/tag_util.h"

#include "scipp/variable/variable_concept.h"
#include "scipp/variable/variable_keyword_arg_constructor.h"

namespace scipp::dataset {
class DataArrayConstView;
template <class T> typename T::view_type makeViewItem(T &);
} // namespace scipp::dataset

namespace scipp::variable {

namespace detail {
void expect0D(const Dimensions &dims);
} // namespace detail

class VariableConstView;
class VariableView;

/// Variable is a type-erased handle to any data structure representing a
/// multi-dimensional array. In addition it has a unit and a set of dimension
/// labels.
class SCIPP_VARIABLE_EXPORT Variable {
public:
  using const_view_type = VariableConstView;
  using view_type = VariableView;

  Variable() = default;
  explicit Variable(const VariableConstView &slice);
  Variable(const Variable &parent, const Dimensions &dims);
  Variable(const VariableConstView &parent, const Dimensions &dims);
  Variable(const Variable &parent, VariableConceptHandle data);
  Variable(VariableConceptHandle data);
  template <class T>
  Variable(const units::Unit unit, const Dimensions &dimensions, T values,
           std::optional<T> variances);
  template <class T, class U>
  explicit Variable(const boost::units::quantity<T, U> &quantity)
      : Variable(quantity.value() * units::Unit(T{})) {}

  /// Keyword-argument constructor.
  ///
  /// This is equivalent to `makeVariable`, except that the dtype is passed at
  /// runtime as first argument instead of a template argument. `makeVariable`
  /// should be prefered where possible, since it generates less code.
  template <class... Ts> Variable(const DType &type, Ts &&... args);

  explicit operator bool() const noexcept { return m_object.operator bool(); }
  Variable operator~() const;

  units::Unit unit() const { return m_unit; }
  void setUnit(const units::Unit &unit) { m_unit = unit; }
  constexpr void expectCanSetUnit(const units::Unit &) const noexcept {}

  Dimensions dims() const && { return m_object->dims(); }
  const Dimensions &dims() const & { return m_object->dims(); }
  void setDims(const Dimensions &dimensions);

  DType dtype() const noexcept { return data().dtype(); }

  bool hasVariances() const noexcept { return data().hasVariances(); }

  template <class T> ElementArrayView<const T> values() const;
  template <class T> ElementArrayView<T> values();
  template <class T> ElementArrayView<const T> variances() const;
  template <class T> ElementArrayView<T> variances();
  template <class T> const auto &value() const {
    detail::expect0D(dims());
    return values<T>()[0];
  }
  template <class T> const auto &variance() const {
    detail::expect0D(dims());
    return variances<T>()[0];
  }
  template <class T> auto &value() {
    detail::expect0D(dims());
    return values<T>()[0];
  }
  template <class T> auto &variance() {
    detail::expect0D(dims());
    return variances<T>()[0];
  }

  // ATTENTION: It is really important to avoid any function returning a
  // (Const)VariableView for rvalue Variable. Otherwise the resulting slice
  // will point to free'ed memory.
  VariableConstView slice(const Slice slice) const &;
  Variable slice(const Slice slice) const &&;
  VariableView slice(const Slice slice) &;
  Variable slice(const Slice slice) &&;

  void rename(const Dim from, const Dim to);

  bool operator==(const VariableConstView &other) const;
  bool operator!=(const VariableConstView &other) const;
  Variable operator-() const;

  Variable &operator+=(const VariableConstView &other) &;
  Variable &operator-=(const VariableConstView &other) &;
  Variable &operator*=(const VariableConstView &other) &;
  Variable &operator/=(const VariableConstView &other) &;

  Variable &operator|=(const VariableConstView &other) &;
  Variable &operator&=(const VariableConstView &other) &;
  Variable &operator^=(const VariableConstView &other) &;

  const VariableConcept &data() const && = delete;
  const VariableConcept &data() const & { return *m_object; }
  VariableConcept &data() && = delete;
  VariableConcept &data() & { return *m_object; }

  void setVariances(Variable v);

  core::element_array_view array_params() const noexcept {
    return {0, dims(), dims(), {}};
  }

  template <class T>
  std::tuple<Variable, Dim, typename T::buffer_type> to_constituents();

private:
  template <class... Ts, class... Args>
  static Variable construct(const DType &type, Args &&... args);

  units::Unit m_unit;
  VariableConceptHandle m_object;
};

/// Factory function for Variable supporting "keyword arguments"
///
/// Two styles are supported:
///     makeVariable<ElementType>(Dims, Shape, Unit, Values<T1>, Variances<T2>)
/// or
///     makeVariable<ElementType>(Dimensions, Unit, Values<T1>, Variances<T2>)
/// Unit, Values, or Variances can be omitted. The order of arguments is
/// arbitrary.
/// Example:
///     makeVariable<float>(units::kg,
///     Shape{1, 2}, Dims{Dim::X, Dim::Y}, Values{3, 4}).
///
/// Relation between Dims, Shape, Dimensions and actual data are as follows:
/// 1. If neither Values nor Variances are provided, resulting Variable contains
///    ONLY values of corresponding length.
/// 2. The Variances can't be provided without any Values.
/// 3. Non empty Values and/or Variances must be consistent with shape.
/// 4. If empty Values and/or Variances are provided, resulting Variable
///    contains default initialized Values and/or Variances, the way to make
///    Variable which contains both Values and Variances given length
///    uninitialised is:
///        makeVariable<T>(Dims{Dim::X}, Shape{5}, Values{}, Variances{});
template <class T, class... Ts> Variable makeVariable(Ts &&... ts) {
  detail::ArgParser<T> parser;
  (parser.parse(std::forward<Ts>(ts)), ...);
  return std::make_from_tuple<Variable>(std::move(parser.args));
}

template <class... Ts, class... Args>
Variable Variable::construct(const DType &type, Args &&... args) {
  std::array vars{core::dtype<Ts> == type
                      ? makeVariable<Ts>(std::forward<Args>(args)...)
                      : Variable()...};
  for (auto &var : vars)
    if (var)
      return std::move(var);
  throw except::TypeError("Unsupported dtype.");
}

template <class... Ts>
Variable::Variable(const DType &type, Ts &&... args)
    : Variable{construct<double, float, int64_t, int32_t, bool, Eigen::Vector3d,
                         Eigen::Matrix3d, std::string, scipp::core::time_point>(
          type, std::forward<Ts>(args)...)} {}

/// Non-mutable view into (a subset of) a Variable.
class SCIPP_VARIABLE_EXPORT VariableConstView {
public:
  using value_type = Variable;

  VariableConstView() = default;
  VariableConstView(const Variable &variable) : m_variable(&variable) {
    if (variable) {
      m_dims = variable.dims();
      m_dataDims = variable.dims();
    }
  }
  VariableConstView(const Variable &variable, const Dimensions &dims);
  VariableConstView(const VariableConstView &slice, const Dim dim,
                    const scipp::index begin, const scipp::index end = -1);

  explicit operator bool() const noexcept {
    return m_variable && m_variable->operator bool();
  }

  auto operator~() const { return m_variable->operator~(); }

  VariableConstView slice(const Slice slice) const;

  VariableConstView transpose(const std::vector<Dim> &dims = {}) const;

  units::Unit unit() const { return m_variable->unit(); }

  const Dimensions &dims() const { return m_dims; }

  std::vector<scipp::index> strides() const;

  DType dtype() const noexcept { return m_variable->dtype(); }

  bool hasVariances() const noexcept { return m_variable->hasVariances(); }

  // Note: This return a view object (a ElementArrayView) that does reference
  // members owner by *this. Therefore we can support this even for
  // temporaries and we do not need to delete the rvalue overload, unlike for
  // many other methods. The data is owned by the underlying variable so it
  // will not be deleted even if *this is a temporary and gets deleted.
  template <class T> ElementArrayView<const T> values() const;
  template <class T> ElementArrayView<const T> variances() const;
  template <class T> const auto &value() const {
    detail::expect0D(dims());
    return values<T>()[0];
  }
  template <class T> const auto &variance() const {
    detail::expect0D(dims());
    return variances<T>()[0];
  }

  bool operator==(const VariableConstView &other) const;
  bool operator!=(const VariableConstView &other) const;
  Variable operator-() const;

  auto &underlying() const { return *m_variable; }
  bool is_trivial() const noexcept;

  core::element_array_view array_params() const noexcept {
    return {m_offset, m_dims, m_dataDims, {}};
  }

  template <class T>
  std::tuple<VariableConstView, Dim, typename T::const_element_type>
  constituents() const;

protected:
  const Variable *m_variable{nullptr};
  scipp::index m_offset{0};
  Dimensions m_dims;
  Dimensions m_dataDims; // not always actual, can be pretend, e.g. with reshape
};

/** Mutable view into (a subset of) a Variable.
 *
 * By inheriting from VariableConstView any code that works for
 * VariableConstView will automatically work also for this mutable variant.*/
class SCIPP_VARIABLE_EXPORT VariableView : public VariableConstView {
public:
  VariableView() = default;
  VariableView(Variable &variable)
      : VariableConstView(variable), m_mutableVariable(&variable) {}
  VariableView(Variable &variable, const Dimensions &dims);
  VariableView(const VariableView &slice, const Dim dim,
               const scipp::index begin, const scipp::index end = -1);

  VariableView slice(const Slice slice) const;

  VariableView transpose(const std::vector<Dim> &dims = {}) const;

  // Note: No need to delete rvalue overloads here, see VariableConstView.
  template <class T> ElementArrayView<T> values() const;
  template <class T> ElementArrayView<T> variances() const;
  template <class T> auto &value() const {
    detail::expect0D(dims());
    return values<T>()[0];
  }
  template <class T> auto &variance() const {
    detail::expect0D(dims());
    return variances<T>()[0];
  }

  // Note: We want to support things like `var(Dim::X, 0) += var2`, i.e., when
  // the left-hand-side is a temporary. This is ok since data is modified in
  // underlying Variable. However, we do not return the typical `VariableView
  // &` from these operations since that could reference a temporary. Due to the
  // way Python implements things like __iadd__ we must return an object
  // referencing the data though. We therefore return by value (this is not for
  // free since it involves a memory allocation but is probably relatively cheap
  // compared to other things). If the return by value turns out to be a
  // performance issue, another option is to have overloads for *this of types
  // `&` and `&&` with distinct return types (by reference in the first case, by
  // value in the second). In principle we may also change the implementation of
  // the Python exports to return `a` after calling `a += b` instead of
  // returning `a += b` but I am not sure how Pybind11 handles object lifetimes
  // (would this suffer from the same issue?).
  template <class T> VariableView assign(const T &other) const;

  VariableView operator+=(const VariableConstView &other) const;
  VariableView operator-=(const VariableConstView &other) const;
  VariableView operator*=(const VariableConstView &other) const;
  VariableView operator/=(const VariableConstView &other) const;

  VariableView operator|=(const VariableConstView &other) const;
  VariableView operator&=(const VariableConstView &other) const;
  VariableView operator^=(const VariableConstView &other) const;

  void setVariances(Variable v) const;

  void setUnit(const units::Unit &unit) const;
  void expectCanSetUnit(const units::Unit &unit) const;

  template <class T>
  std::tuple<VariableConstView, Dim, typename T::element_type>
  constituents() const;

  template <class T> void replace_model(T model) const;

private:
  friend class dataset::DataArrayConstView;
  template <class T> friend typename T::view_type dataset::makeViewItem(T &);

  // For internal use in DataArrayConstView.
  explicit VariableView(VariableConstView &&base)
      : VariableConstView(std::move(base)), m_mutableVariable{nullptr} {}

  Variable *m_mutableVariable{nullptr};
};

SCIPP_VARIABLE_EXPORT Variable copy(const VariableConstView &var);
SCIPP_VARIABLE_EXPORT VariableView copy(const VariableConstView &dataset,
                                        const VariableView &out);

} // namespace scipp::variable

namespace scipp {
using variable::Dims;
using variable::makeVariable;
using variable::Shape;
using variable::Values;
using variable::Variable;
using variable::VariableConstView;
using variable::VariableView;
using variable::Variances;
template <class T> struct is_view : std::false_type {};
template <class T> inline constexpr bool is_view_v = is_view<T>::value;
template <> struct is_view<VariableConstView> : std::true_type {};
template <> struct is_view<VariableView> : std::true_type {};
} // namespace scipp
