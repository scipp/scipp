// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <Eigen/Core>

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
#include "scipp/core/strides.h"
#include "scipp/core/tag_util.h"

#include "scipp/variable/variable_concept.h"
#include "scipp/variable/variable_keyword_arg_constructor.h"

namespace llnl::units {
class precise_measurement;
}

namespace scipp::dataset {
class DataArrayConstView;
template <class T> typename T::view_type makeViewItem(T &);
} // namespace scipp::dataset

namespace scipp::variable {

namespace detail {
SCIPP_VARIABLE_EXPORT void expect0D(const Dimensions &dims);
} // namespace detail

class VariableConstView;

} // namespace scipp::variable

namespace scipp {
template <class T> struct is_view : std::false_type {};
template <class T> inline constexpr bool is_view_v = is_view<T>::value;
template <> struct is_view<variable::VariableConstView> : std::true_type {};
} // namespace scipp

namespace scipp::variable {

/// Variable is a type-erased handle to any data structure representing a
/// multi-dimensional array. In addition it has a unit and a set of dimension
/// labels.
class SCIPP_VARIABLE_EXPORT Variable {
public:
  using const_view_type = Variable;
  using view_type = Variable;

  Variable() = default;
  explicit Variable(const VariableConstView &slice);
  Variable(const Variable &parent, const Dimensions &dims);
  Variable(const VariableConstView &parent, const Dimensions &dims);
  Variable(const VariableConstView &parent, const Dimensions &dims,
           VariableConceptHandle data);
  Variable(const Dimensions &dims, VariableConceptHandle data);
  template <class T>
  Variable(const units::Unit unit, const Dimensions &dimensions, T values,
           std::optional<T> variances);
  explicit Variable(const llnl::units::precise_measurement &m);

  Variable &assign(const VariableConstView &other);

  /// Keyword-argument constructor.
  ///
  /// This is equivalent to `makeVariable`, except that the dtype is passed at
  /// runtime as first argument instead of a template argument. `makeVariable`
  /// should be prefered where possible, since it generates less code.
  template <class... Ts> Variable(const DType &type, Ts &&... args);

  explicit operator bool() const noexcept { return m_object.operator bool(); }
  Variable operator~() const;

  units::Unit unit() const { return m_object->unit(); }
  void setUnit(const units::Unit &unit);
  void expectCanSetUnit(const units::Unit &) const;

  Dimensions dims() const && { return m_dims; }
  const Dimensions &dims() const & { return m_dims; }
  void setDims(const Dimensions &dimensions);

  DType dtype() const noexcept { return data().dtype(); }

  scipp::span<const scipp::index> strides() const;

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

  Variable slice(const Slice slice) const;

  void rename(const Dim from, const Dim to);

  bool operator==(const VariableConstView &other) const;
  bool operator!=(const VariableConstView &other) const;
  Variable operator-() const;

  const VariableConcept &data() const && = delete;
  const VariableConcept &data() const & { return *m_object; }
  VariableConcept &data() && = delete;
  VariableConcept &data() & { return *m_object; }

  void setVariances(const Variable &v);

  core::ElementArrayViewParams array_params() const noexcept;

  VariableConstView bin_indices() const;

  template <class T>
  std::tuple<Variable, Dim, typename T::const_element_type>
  constituents() const;
  template <class T>
  std::tuple<Variable, Dim, typename T::element_type> constituents();

  template <class T>
  std::tuple<Variable, Dim, typename T::buffer_type> to_constituents();

  [[nodiscard]] Variable transpose(const std::vector<Dim> &order) const;

private:
  template <class... Ts, class... Args>
  static Variable construct(const DType &type, Args &&... args);

  Dimensions m_dims;
  Strides m_strides;
  scipp::index m_offset{0};
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
  using const_view_type = VariableConstView;
  using view_type = VariableConstView;

  VariableConstView() = default;
  VariableConstView(const Variable &variable) : m_variable(&variable) {
    if (variable) {
      m_dims = variable.dims();
      m_dataDims = variable.array_params().dataDims();
      m_offset = variable.array_params().offset();
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

  void rename(const Dim from, const Dim to);

  core::ElementArrayViewParams array_params() const noexcept {
    return {m_offset, m_dims, m_dataDims, {}};
  }

  VariableConstView bin_indices() const;

  template <class T>
  std::tuple<Variable, Dim, typename T::const_element_type>
  constituents() const;

protected:
  const Variable *m_variable{nullptr};
  scipp::index m_offset{0};
  Dimensions m_dims;
  Dimensions m_dataDims; // not always actual, can be pretend, e.g. with reshape
};

SCIPP_VARIABLE_EXPORT Variable copy(const VariableConstView &var);
SCIPP_VARIABLE_EXPORT Variable &copy(const VariableConstView &dataset,
                                     Variable &out);

} // namespace scipp::variable

namespace scipp::core {
template <> inline constexpr DType dtype<variable::Variable>{1000};
template <> inline constexpr DType dtype<bucket<variable::Variable>>{1001};
} // namespace scipp::core

namespace scipp {
using variable::Dims;
using variable::makeVariable;
using variable::Shape;
using variable::Values;
using variable::Variable;
using variable::VariableConstView;
using variable::Variances;
} // namespace scipp

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/logical.h"
