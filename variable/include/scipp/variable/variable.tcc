// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dimensions.h"
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"
#include "scipp/units/unit.h"
#include "scipp/variable/apply.h"
#include "scipp/variable/except.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/transform.h"
#include <numeric>
#include <optional>

namespace scipp::variable {

template <class T, class C> auto &requireT(C &concept) {
  try {
    return dynamic_cast<T &>(concept);
  } catch (const std::bad_cast &) {
    throw except::TypeError("Expected item dtype " +
                            to_string(T::static_dtype()) + ", got " +
                            to_string(concept.dtype()) + '.');
  }
}

template <class T> struct is_span : std::false_type {};
template <class T> struct is_span<scipp::span<T>> : std::true_type {};
template <class T> inline constexpr bool is_span_v = is_span<T>::value;

template <class T1, class T2> bool equal(const T1 &view1, const T2 &view2) {
  // TODO Use optimizations in case of contigous views (instead of slower
  // ElementArrayView iteration). Add multi threading?
  if constexpr (is_span_v<typename T1::value_type>)
    return std::equal(view1.begin(), view1.end(), view2.begin(), view2.end(),
                      [](auto &a, auto &b) { return equal(a, b); });
  else
    return std::equal(view1.begin(), view1.end(), view2.begin(), view2.end());
}

template <class T, class... Args>
auto optionalVariancesView(T &concept, Args &&... args) {
  return concept.hasVariances()
             ? std::optional(concept.variances(std::forward<Args>(args)...))
             : std::nullopt;
}


template <class T> class DataModel;

template <class T>
VariableConceptHandle
VariableConceptT<T>::makeDefaultFromParent(const Dimensions &dims) const {
  using TT = element_array<std::decay_t<T>>;
  if (hasVariances())
    return std::make_unique<DataModel<TT>>(dims, TT(dims.volume()),
                                           TT(dims.volume()));
  else
    return std::make_unique<DataModel<TT>>(dims, TT(dims.volume()));
}

template <class T, class... Args>
auto optionalVariancesReshaped(T &concept, Args &&... args) {
  return concept.hasVariances() ? std::optional(concept.variancesReshaped(
                                      std::forward<Args>(args)...))
                                : std::nullopt;
}

/// Helper for implementing Variable(View)::operator==.
///
/// This method is using virtual dispatch as a trick to obtain T, such that
/// values<T> and variances<T> can be compared.
template <class T>
bool VariableConceptT<T>::equals(const VariableConstView &a, const VariableConstView &b) const {
  if (a.unit() != b.unit())
    return false;
  if (a.dims() != b.dims())
    return false;
  if (a.dtype() != b.dtype())
    return false;
  if (a.hasVariances() != b.hasVariances())
    return false;
  if (a.dims().volume() == 0 && a.dims() == b.dims())
    return true;
  return equal(a.values<T>(), b.values<T>()) &&
             (!a.hasVariances() ||
              equal(a.variances<T>(), b.variances<T>()));
}

/// Helper for implementing Variable(View) copy operations.
///
/// This method is using virtual dispatch as a trick to obtain T, such that
/// transform can be called with any T.
template <class T>
void VariableConceptT<T>::copy(const VariableConstView &src,
                               const VariableView &dest) const {
  transform_in_place<T>(
      dest, src,
      overloaded{core::transform_flags::no_event_list_handling,
                 core::transform_flags::expect_in_variance_if_out_variance,
                 [](auto &a, const auto &b) { a = b; }});
}

namespace {
template <class T> struct default_init {
  static T value() { return T(); }
};
// Eigen does not zero-initialize matrices (vectors), which is a recurrent
// source of bugs. Variable does zero-init instead.
template <class T, int Rows, int Cols>
struct default_init<Eigen::Matrix<T, Rows, Cols>> {
  static Eigen::Matrix<T, Rows, Cols> value() {
    return Eigen::Matrix<T, Rows, Cols>::Zero();
  }
};
} // namespace

/// Implementation of VariableConcept that holds data.
template <class T>
class DataModel : public VariableConceptT<typename T::value_type> {
public:
  using value_type = std::remove_const_t<typename T::value_type>;

  DataModel(const Dimensions &dimensions, T model,
            std::optional<T> variances = std::nullopt)
      : VariableConceptT<typename T::value_type>(dimensions),
        m_values(
            model ? std::move(model)
                  : T(dimensions.volume(), default_init<value_type>::value())),
        m_variances(std::move(variances)) {
    if (m_variances && !core::canHaveVariances<value_type>())
      throw except::VariancesError("This data type cannot have variances.");
    if (this->dims().volume() != scipp::size(m_values))
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
    if (m_variances && !*m_variances)
      *m_variances = T(dimensions.volume(), default_init<value_type>::value());
  }

  void setVariances(Variable &&variances) override {
    if (!core::canHaveVariances<value_type>())
      throw except::VariancesError("This data type cannot have variances.");
    if (!variances)
      return m_variances.reset();
    if (variances.hasVariances())
      throw except::VariancesError(
          "Cannot set variances from variable with variances.");
    core::expect::equals(this->dims(), variances.dims());
    m_variances.emplace(
        std::move(requireT<DataModel>(variances.data()).m_values));
  }

  ElementArrayView<value_type> values(const Dimensions &dims) override {
    return makeElementArrayView(m_values.data(), 0, dims, this->dims());
  }
  ElementArrayView<value_type> values(const Dimensions &dims, const Dim dim,
                                          const scipp::index begin) override {
    scipp::index beginOffset = this->dims().contains(dim)
                                   ? begin * this->dims().offset(dim)
                                   : begin * this->dims().volume();
    return makeElementArrayView(m_values.data(), beginOffset, dims,
                                this->dims());
  }

  ElementArrayView<const value_type>
  values(const Dimensions &dims) const override {
    return makeElementArrayView(m_values.data(), 0, dims, this->dims());
  }
  ElementArrayView<const value_type>
  values(const Dimensions &dims, const Dim dim,
             const scipp::index begin) const override {
    scipp::index beginOffset = this->dims().contains(dim)
                                   ? begin * this->dims().offset(dim)
                                   : begin * this->dims().volume();
    return makeElementArrayView(m_values.data(), beginOffset, dims,
                                this->dims());
  }

  ElementArrayView<value_type> variances(const Dimensions &dims) override {
    return makeElementArrayView(m_variances->data(), 0, dims, this->dims());
  }
  ElementArrayView<value_type>
  variances(const Dimensions &dims, const Dim dim,
                const scipp::index begin) override {
    scipp::index beginOffset = this->dims().contains(dim)
                                   ? begin * this->dims().offset(dim)
                                   : begin * this->dims().volume();
    return makeElementArrayView(m_variances->data(), beginOffset, dims,
                                this->dims());
  }

  ElementArrayView<const value_type>
  variances(const Dimensions &dims) const override {
    return makeElementArrayView(m_variances->data(), 0, dims, this->dims());
  }
  ElementArrayView<const value_type>
  variances(const Dimensions &dims, const Dim dim,
                const scipp::index begin) const override {
    scipp::index beginOffset = this->dims().contains(dim)
                                   ? begin * this->dims().offset(dim)
                                   : begin * this->dims().volume();
    return makeElementArrayView(m_variances->data(), beginOffset, dims,
                                this->dims());
  }

  ElementArrayView<const value_type>
  valuesReshaped(const Dimensions &dims) const override {
    return makeElementArrayView(m_values.data(), 0, dims, dims);
  }
  ElementArrayView<value_type> valuesReshaped(const Dimensions &dims) override {
    return makeElementArrayView(m_values.data(), 0, dims, dims);
  }

  ElementArrayView<const value_type>
  variancesReshaped(const Dimensions &dims) const override {
    return makeElementArrayView(m_variances->data(), 0, dims, dims);
  }
  ElementArrayView<value_type>
  variancesReshaped(const Dimensions &dims) override {
    return makeElementArrayView(m_variances->data(), 0, dims, dims);
  }

  std::unique_ptr<VariableConceptT<typename T::value_type>>
  copyT() const override {
    return std::make_unique<DataModel<T>>(*this);
  }

  VariableConceptHandle clone() const override {
    return std::make_unique<DataModel<T>>(this->dims(), m_values, m_variances);
  }

  bool isContiguous() const override { return true; }
  bool isView() const override { return false; }
  bool isConstView() const override { return false; }
  bool hasVariances() const noexcept override {
    return m_variances.has_value();
  }

  scipp::index size() const override { return m_values.size(); }

  T m_values;
  std::optional<T> m_variances;
};

template <class T>
Variable::Variable(const units::Unit unit, const Dimensions &dimensions,
                   T values_, std::optional<T> variances_)
    : m_unit{unit},
      m_object(std::make_unique<DataModel<T>>(
          std::move(dimensions), std::move(values_), std::move(variances_))) {}

template <class T>
const element_array<T> &Variable::cast(const bool variances_) const {
  auto &dm = requireT<const DataModel<element_array<T>>>(*m_object);
  if (!variances_)
    return dm.m_values;
  else {
    core::expect::hasVariances(*this);
    return *dm.m_variances;
  }
}

template <class T> element_array<T> &Variable::cast(const bool variances_) {
  auto &dm = requireT<DataModel<element_array<T>>>(*m_object);
  if (!variances_)
    return dm.m_values;
  else {
    core::expect::hasVariances(*this);
    return *dm.m_variances;
  }
}

template <class T>
const ElementArrayView<const T> VariableConstView::cast() const {
  return {&m_variable->values<T>()[0], m_offset, m_dims, m_dataDims};
}

template <class T>
const ElementArrayView<const T> VariableConstView::castVariances() const {
  core::expect::hasVariances(*this);
  return {&m_variable->variances<T>()[0], m_offset, m_dims, m_dataDims};
}

template <class T> ElementArrayView<T> VariableView::cast() const {
  return {&m_mutableVariable->values<T>()[0], m_offset, m_dims, m_dataDims};
}

template <class T> ElementArrayView<T> VariableView::castVariances() const {
  core::expect::hasVariances(*this);
  return {&m_mutableVariable->variances<T>()[0], m_offset, m_dims, m_dataDims};
}

/// Macro for instantiating classes and functions required for support a new
/// dtype in Variable.
#define INSTANTIATE_VARIABLE(name, ...)                                        \
  namespace {                                                                  \
  auto register_dtype_name_##name(                                             \
      (core::dtypeNameRegistry().emplace(dtype<__VA_ARGS__>, #name), 0));      \
  }                                                                            \
  template Variable::Variable(const units::Unit, const Dimensions &,           \
                              element_array<__VA_ARGS__>,                      \
                              std::optional<element_array<__VA_ARGS__>>);      \
  template element_array<__VA_ARGS__> &Variable::cast<__VA_ARGS__>(            \
      const bool);                                                             \
  template const element_array<__VA_ARGS__> &Variable::cast<__VA_ARGS__>(      \
      const bool) const;                                                       \
  template const ElementArrayView<const __VA_ARGS__>                           \
  VariableConstView::cast<__VA_ARGS__>() const;                                \
  template const ElementArrayView<const __VA_ARGS__>                           \
  VariableConstView::castVariances<__VA_ARGS__>() const;                       \
  template ElementArrayView<__VA_ARGS__> VariableView::cast<__VA_ARGS__>()     \
      const;                                                                   \
  template ElementArrayView<__VA_ARGS__>                                       \
  VariableView::castVariances<__VA_ARGS__>() const;

} // namespace scipp::variable
