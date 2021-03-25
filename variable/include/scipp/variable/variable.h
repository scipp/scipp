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

/// Variable is a type-erased handle to any data structure representing a
/// multi-dimensional array. In addition it has a unit and a set of dimension
/// labels.
class SCIPP_VARIABLE_EXPORT Variable {
public:
  using const_view_type = Variable;
  using view_type = Variable;

  Variable() = default;
  Variable(const Variable &parent, const Dimensions &dims);
  Variable(const Dimensions &dims, VariableConceptHandle data);
  template <class T>
  Variable(const units::Unit unit, const Dimensions &dimensions, T values,
           std::optional<T> variances);
  explicit Variable(const llnl::units::precise_measurement &m);

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

  bool operator==(const Variable &other) const;
  bool operator!=(const Variable &other) const;
  Variable operator-() const;

  const VariableConcept &data() const && = delete;
  const VariableConcept &data() const & { return *m_object; }
  VariableConcept &data() && = delete;
  VariableConcept &data() & { return *m_object; }
  const auto &data_handle() const { return m_object; }
  void setDataHandle(VariableConceptHandle object);

  void setVariances(const Variable &v);

  core::ElementArrayViewParams array_params() const noexcept;

  Variable bin_indices() const;

  template <class T>
  std::tuple<Variable, Dim, typename T::const_element_type>
  constituents() const;
  template <class T>
  std::tuple<Variable, Dim, typename T::element_type> constituents();

  template <class T>
  std::tuple<Variable, Dim, typename T::buffer_type> to_constituents();

  [[nodiscard]] Variable transpose(const std::vector<Dim> &order) const;

  bool is_slice() const;

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
  throw except::TypeError("Unsupported dtype for constructing a Variable: " +
                          to_string(type));
}

template <class... Ts>
Variable::Variable(const DType &type, Ts &&... args)
    : Variable{construct<double, float, int64_t, int32_t, bool, Eigen::Vector3d,
                         Eigen::Matrix3d, std::string, scipp::core::time_point>(
          type, std::forward<Ts>(args)...)} {}

SCIPP_VARIABLE_EXPORT Variable copy(const Variable &var);
SCIPP_VARIABLE_EXPORT Variable &copy(const Variable &dataset, Variable &out);
SCIPP_VARIABLE_EXPORT Variable copy(const Variable &dataset, Variable &&out);

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
using variable::Variances;
} // namespace scipp

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/logical.h"
