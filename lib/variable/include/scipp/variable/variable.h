// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "scipp-variable_export.h"
#include "scipp/common/index.h"
#include "scipp/units/unit.h"

#include "scipp/core/dimensions.h"
#include "scipp/core/dtype.h"
#include "scipp/core/element_array.h"
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"
#include "scipp/core/slice.h"
#include "scipp/core/strides.h"

#include "scipp/variable/variable_keyword_arg_constructor.h"

namespace llnl::units {
class precise_measurement;
}

namespace scipp::variable {

class VariableConcept;
using VariableConceptHandle = std::shared_ptr<VariableConcept>;

/// Variable is a type-erased handle to any data structure representing a
/// multi-dimensional array. In addition it has a unit and a set of dimension
/// labels.
class SCIPP_VARIABLE_EXPORT Variable {
public:
  Variable() = default;
  Variable(const Variable &parent, const Dimensions &dims);
  Variable(const Dimensions &dims, VariableConceptHandle data);
  template <class T>
  Variable(units::Unit unit, const Dimensions &dimensions, T values,
           std::optional<T> variances);
  explicit Variable(const llnl::units::precise_measurement &m);

  /// Keyword-argument constructor.
  ///
  /// This is equivalent to `makeVariable`, except that the dtype is passed at
  /// runtime as first argument instead of a template argument. `makeVariable`
  /// should be prefered where possible, since it generates less code.
  template <class... Ts> Variable(const DType &type, Ts &&... args);

  Variable(const Variable &other) = default;
  Variable(Variable &&other) noexcept = default;

  Variable &operator=(const Variable &other);
  Variable &operator=(Variable &&other);

  ~Variable() noexcept = default;

  [[nodiscard]] const units::Unit &unit() const;
  void setUnit(const units::Unit &unit);
  void expect_can_set_unit(const units::Unit &unit) const;

  [[nodiscard]] const Dimensions &dims() const;
  [[nodiscard]] Dim dim() const;
  [[nodiscard]] scipp::index ndim() const;

  [[nodiscard]] DType dtype() const;

  [[nodiscard]] scipp::span<const scipp::index> strides() const;
  [[nodiscard]] scipp::index stride(const Dim dim) const;
  [[nodiscard]] scipp::index offset() const;

  [[nodiscard]] bool has_variances() const;

  template <class T> ElementArrayView<const T> values() const;
  template <class T> ElementArrayView<T> values();
  template <class T> ElementArrayView<const T> variances() const;
  template <class T> ElementArrayView<T> variances();
  template <class T> const auto &value() const {
    core::expect::ndim_is(dims(), 0);
    return values<T>()[0];
  }
  template <class T> const auto &variance() const {
    core::expect::ndim_is(dims(), 0);
    return variances<T>()[0];
  }
  template <class T> auto &value() {
    core::expect::ndim_is(dims(), 0);
    return values<T>()[0];
  }
  template <class T> auto &variance() {
    core::expect::ndim_is(dims(), 0);
    return variances<T>()[0];
  }

  [[nodiscard]] Variable slice(Slice params) const;
  void validateSlice(const Slice &s, const Variable &data) const;
  [[maybe_unused]] Variable &setSlice(Slice params, const Variable &data);

  template <class T> Variable elements() const;
  template <class T> Variable elements(const std::string &key) const;

  void rename(Dim from, Dim to);

  bool operator==(const Variable &other) const;
  bool operator!=(const Variable &other) const;

  [[nodiscard]] const VariableConcept &data() const && = delete;
  [[nodiscard]] const VariableConcept &data() const &;
  VariableConcept &data() && = delete;
  VariableConcept &data() &;
  [[nodiscard]] const VariableConceptHandle &data_handle() const;
  void setDataHandle(VariableConceptHandle object);

  void setVariances(const Variable &v);

  [[nodiscard]] core::ElementArrayViewParams array_params() const;

  [[nodiscard]] Variable bin_indices() const;
  template <class T> const T &bin_buffer() const;
  template <class T> T &bin_buffer();

  template <class T> std::tuple<Variable, Dim, T> constituents() const;
  template <class T> std::tuple<Variable, Dim, T> constituents();
  template <class T> std::tuple<Variable, Dim, T> to_constituents();

  [[nodiscard]] Variable broadcast(const Dimensions &target) const;
  [[nodiscard]] Variable fold(const Dim dim, const Dimensions &target) const;
  [[nodiscard]] Variable transpose(scipp::span<const Dim> order) const;

  [[nodiscard]] bool is_valid() const noexcept;
  [[nodiscard]] bool is_slice() const;
  [[nodiscard]] bool is_readonly() const noexcept;
  [[nodiscard]] bool is_same(const Variable &other) const noexcept;

  [[nodiscard]] Variable as_const() const;

  auto &unchecked_dims() { return m_dims; }
  auto &unchecked_strides() { return m_strides; }

private:
  // Declared friend so gtest recognizes it
  friend SCIPP_VARIABLE_EXPORT std::ostream &operator<<(std::ostream &,
                                                        const Variable &);
  template <class... Ts, class... Args>
  static Variable construct(const DType &type, Args &&... args);
  template <class T, class... Index>
  Variable elements_impl(Index... index) const;

  void expect_writable() const;

  Dimensions m_dims;
  Strides m_strides;
  scipp::index m_offset{0};
  VariableConceptHandle m_object;
  bool m_readonly{false};
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
///    uninitialized is:
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
    if (var.is_valid())
      return var;
  throw except::TypeError("Unsupported dtype for constructing a Variable: " +
                          to_string(type));
}

template <class... Ts>
Variable::Variable(const DType &type, Ts &&... args)
    : Variable{construct<double, float, int64_t, int32_t, bool, std::string,
                         scipp::core::time_point>(type,
                                                  std::forward<Ts>(args)...)} {}

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable copy(const Variable &var);
[[maybe_unused]] SCIPP_VARIABLE_EXPORT Variable &copy(const Variable &var,
                                                      Variable &out);
[[maybe_unused]] SCIPP_VARIABLE_EXPORT Variable copy(const Variable &var,
                                                     Variable &&out);
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
