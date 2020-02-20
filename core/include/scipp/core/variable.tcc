// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold

#include "scipp/core/apply.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/variable.h"
#include "scipp/core/element_array_view.h"
#include "scipp/units/unit.h"
#include <numeric>
#include <optional>

namespace scipp::core {

template <class T, class C> auto &requireT(C &concept) {
  try {
    return dynamic_cast<T &>(concept);
  } catch (const std::bad_cast &) {
    throw except::TypeError("Expected item dtype " +
                            to_string(T::static_dtype()) + ", got " +
                            to_string(concept.dtype()) + '.');
  }
}

template <class T1, class T2> bool equal(const T1 &view1, const T2 &view2) {
  return std::equal(view1.begin(), view1.end(), view2.begin(), view2.end());
}

template <class T> class ViewModel;

template <class T>
auto makeSpan(T &model, const Dimensions &dims, const Dim dim,
              const scipp::index begin, const scipp::index end) {
  if (!dims.denseContains(dim) && (begin != 0 || end != 1))
    throw std::runtime_error("VariableConcept: Slice index out of range.");
  if (!dims.denseContains(dim) || dims[dim] == end - begin) {
    return scipp::span(model.data(), model.data() + model.size());
  }
  const scipp::index beginOffset = begin * dims.offset(dim);
  const scipp::index endOffset = end * dims.offset(dim);
  return scipp::span(model.data() + beginOffset, model.data() + endOffset);
}

template <class T, class... Args>
auto optionalVariancesView(T &concept, Args &&... args) {
  return concept.hasVariances()
             ? std::optional(concept.variancesView(std::forward<Args>(args)...))
             : std::nullopt;
}

template <class T> class DataModel;

template <class T>
VariableConceptHandle
VariableConceptT<T>::makeDefaultFromParent(const Dimensions &dims) const {
  using TT = detail::element_array<std::decay_t<T>>;
  if (hasVariances())
    return std::make_unique<DataModel<TT>>(dims, TT(dims.volume()),
                                           TT(dims.volume()));
  else
    return std::make_unique<DataModel<TT>>(dims, TT(dims.volume()));
}

template <class T> VariableConceptHandle VariableConceptT<T>::makeView() const {
  auto &dims = this->dims();
  return std::make_unique<ViewModel<decltype(valuesView(dims))>>(
      dims, valuesView(dims), optionalVariancesView(*this, dims));
}

template <class T> VariableConceptHandle VariableConceptT<T>::makeView() {
  if (this->isConstView())
    return const_cast<const VariableConceptT &>(*this).makeView();
  auto &dims = this->dims();
  return std::make_unique<ViewModel<decltype(valuesView(dims))>>(
      dims, valuesView(dims), optionalVariancesView(*this, dims));
}

template <class T>
VariableConceptHandle
VariableConceptT<T>::makeView(const Dim dim, const scipp::index begin,
                              const scipp::index end) const {
  auto dims = this->dims();
  if (end == -1)
    dims.erase(dim);
  else
    dims.resize(dim, end - begin);
  return std::make_unique<ViewModel<decltype(valuesView(dims, dim, begin))>>(
      dims, valuesView(dims, dim, begin),
      optionalVariancesView(*this, dims, dim, begin));
}

template <class T>
VariableConceptHandle VariableConceptT<T>::makeView(const Dim dim,
                                                    const scipp::index begin,
                                                    const scipp::index end) {
  if (this->isConstView())
    return const_cast<const VariableConceptT &>(*this).makeView(dim, begin,
                                                                end);
  auto dims = this->dims();
  if (end == -1)
    dims.erase(dim);
  else
    dims.resize(dim, end - begin);
  return std::make_unique<ViewModel<decltype(valuesView(dims, dim, begin))>>(
      dims, valuesView(dims, dim, begin),
      optionalVariancesView(*this, dims, dim, begin));
}

template <class T, class... Args>
auto optionalVariancesReshaped(T &concept, Args &&... args) {
  return concept.hasVariances() ? std::optional(concept.variancesReshaped(
                                      std::forward<Args>(args)...))
                                : std::nullopt;
}

template <class T>
VariableConceptHandle
VariableConceptT<T>::reshape(const Dimensions &dims) const {
  if (this->dims().volume() != dims.volume())
    throw std::runtime_error(
        "Cannot reshape to dimensions with different volume");
  return std::make_unique<ViewModel<decltype(valuesReshaped(dims))>>(
      dims, valuesReshaped(dims), optionalVariancesReshaped(*this, dims));
}

template <class T>
VariableConceptHandle VariableConceptT<T>::reshape(const Dimensions &dims) {
  if (this->dims().volume() != dims.volume())
    throw std::runtime_error(
        "Cannot reshape to dimensions with different volume");
  return std::make_unique<ViewModel<decltype(valuesReshaped(dims))>>(
      dims, valuesReshaped(dims), optionalVariancesReshaped(*this, dims));
}

template <class T>
VariableConceptHandle
VariableConceptT<T>::transpose(const std::vector<Dim> &tdims) const {
  auto dms = Dimensions(std::vector<Dim>(tdims.begin(), tdims.end()),
                        detail::reorderedShape(tdims, dims()));
  using U = decltype(valuesView(dms));
  return std::make_unique<ViewModel<U>>(
      dms, valuesView(dms),
      this->hasVariances() ? std::optional(variancesView(dms)) : std::nullopt);
}

template <class T>
VariableConceptHandle
VariableConceptT<T>::transpose(const std::vector<Dim> &tdims) {
  auto dms = Dimensions(std::vector<Dim>(tdims.begin(), tdims.end()),
                        detail::reorderedShape(tdims, dims()));
  using U = decltype(valuesView(dms));
  return std::make_unique<ViewModel<U>>(
      dms, valuesView(dms),
      this->hasVariances() ? std::optional(variancesView(dms)) : std::nullopt);
}

template <class T>
bool VariableConceptT<T>::operator==(const VariableConcept &other) const {
  const auto &dims = this->dims();
  if (dims != other.dims())
    return false;
  if (this->dtype() != other.dtype())
    return false;
  if (this->hasVariances() != other.hasVariances())
    return false;
  const auto &otherT = requireT<const VariableConceptT>(other);
  if (dims.volume() == 0 && dims == other.dims())
    return true;
  if (this->isContiguous()) {
    if (other.isContiguous() && dims.isContiguousIn(other.dims())) {
      return equal(values(), otherT.values()) &&
             (!this->hasVariances() || equal(variances(), otherT.variances()));
    } else {
      return equal(values(), otherT.valuesView(dims)) &&
             (!this->hasVariances() ||
              equal(variances(), otherT.variancesView(dims)));
    }
  } else {
    if (other.isContiguous() && dims.isContiguousIn(other.dims())) {
      return equal(valuesView(dims), otherT.values()) &&
             (!this->hasVariances() ||
              equal(variancesView(dims), otherT.variances()));
    } else {
      return equal(valuesView(dims), otherT.valuesView(dims)) &&
             (!this->hasVariances() ||
              equal(variancesView(dims), otherT.variancesView(dims)));
    }
  }
}

template <class T>
void VariableConceptT<T>::copy(const VariableConcept &other, const Dim dim,
                               const scipp::index offset,
                               const scipp::index otherBegin,
                               const scipp::index otherEnd) {
  if (this->hasVariances() != other.hasVariances())
    throw except::VariancesError(
        "Either both or neither of the operands must have a variances.");
  auto iterDims = this->dims();
  const scipp::index delta = otherEnd - otherBegin;
  if (iterDims.contains(dim))
    iterDims.resize(dim, delta);

  const auto &otherT = requireT<const VariableConceptT>(other);
  auto otherView = otherT.valuesView(iterDims, dim, otherBegin);
  // Four cases for minimizing use of ElementArrayView --- just copy contiguous
  // range where possible.
  if (this->isContiguous() && iterDims.isContiguousIn(this->dims())) {
    auto target = values(dim, offset, offset + delta);
    if (other.isContiguous() && iterDims.isContiguousIn(other.dims())) {
      auto source = otherT.values(dim, otherBegin, otherEnd);
      std::copy(source.begin(), source.end(), target.begin());
    } else {
      std::copy(otherView.begin(), otherView.end(), target.begin());
    }
  } else {
    auto view = valuesView(iterDims, dim, offset);
    if (other.isContiguous() && iterDims.isContiguousIn(other.dims())) {
      auto source = otherT.values(dim, otherBegin, otherEnd);
      std::copy(source.begin(), source.end(), view.begin());
    } else {
      std::copy(otherView.begin(), otherView.end(), view.begin());
    }
  }
  // TODO Avoid code duplication for variances.
  if (this->hasVariances()) {
    auto otherVariances = otherT.variancesView(iterDims, dim, otherBegin);
    if (this->isContiguous() && iterDims.isContiguousIn(this->dims())) {
      auto target = variances(dim, offset, offset + delta);
      if (other.isContiguous() && iterDims.isContiguousIn(other.dims())) {
        auto source = otherT.variances(dim, otherBegin, otherEnd);
        std::copy(source.begin(), source.end(), target.begin());
      } else {
        std::copy(otherVariances.begin(), otherVariances.end(), target.begin());
      }
    } else {
      auto view = variancesView(iterDims, dim, offset);
      if (other.isContiguous() && iterDims.isContiguousIn(other.dims())) {
        auto source = otherT.variances(dim, otherBegin, otherEnd);
        std::copy(source.begin(), source.end(), view.begin());
      } else {
        std::copy(otherVariances.begin(), otherVariances.end(), view.begin());
      }
    }
  }
}

/// Implementation of VariableConcept that holds data.
template <class T>
class DataModel : public VariableConceptT<typename T::value_type> {
public:
  using value_type = std::remove_const_t<typename T::value_type>;

  DataModel(const Dimensions &dimensions, T model,
            std::optional<T> variances = std::nullopt)
      : VariableConceptT<typename T::value_type>(std::move(dimensions)),
        m_values(std::move(model)), m_variances(std::move(variances)) {
    if (m_variances && !canHaveVariances<value_type>())
      throw except::VariancesError("This data type cannot have variances.");
    if (this->dims().volume() != scipp::size(m_values))
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
  }

  void setVariances(Variable &&variances) override {
    if (!canHaveVariances<value_type>())
      throw except::VariancesError("This data type cannot have variances.");
    if (!variances)
      return m_variances.reset();
    if (variances.hasVariances())
      throw except::VariancesError(
          "Cannot set variances from variable with variances.");
    expect::equals(this->dims(), variances.dims());
    m_variances.emplace(
        std::move(requireT<DataModel>(variances.data()).m_values));
  }

  scipp::span<value_type> values() override {
    return scipp::span(m_values.data(), m_values.data() + size());
  }
  scipp::span<value_type> values(const Dim dim, const scipp::index begin,
                                 const scipp::index end) override {
    return makeSpan(m_values, this->dims(), dim, begin, end);
  }

  scipp::span<const value_type> values() const override {
    return scipp::span(m_values.data(), m_values.data() + size());
  }
  scipp::span<const value_type> values(const Dim dim, const scipp::index begin,
                                       const scipp::index end) const override {
    return makeSpan(m_values, this->dims(), dim, begin, end);
  }

  scipp::span<value_type> variances() override {
    return scipp::span(m_variances->data(), m_variances->data() + size());
  }
  scipp::span<value_type> variances(const Dim dim, const scipp::index begin,
                                    const scipp::index end) override {
    return makeSpan(*m_variances, this->dims(), dim, begin, end);
  }

  scipp::span<const value_type> variances() const override {
    return scipp::span(m_variances->data(), m_variances->data() + size());
  }
  scipp::span<const value_type>
  variances(const Dim dim, const scipp::index begin,
            const scipp::index end) const override {
    return makeSpan(*m_variances, this->dims(), dim, begin, end);
  }

  ElementArrayView<value_type> valuesView(const Dimensions &dims) override {
    return makeElementArrayView(m_values.data(), 0, dims, this->dims());
  }
  ElementArrayView<value_type> valuesView(const Dimensions &dims, const Dim dim,
                                          const scipp::index begin) override {
    scipp::index beginOffset = this->dims().contains(dim)
                                   ? begin * this->dims().offset(dim)
                                   : begin * this->dims().volume();
    return makeElementArrayView(m_values.data(), beginOffset, dims,
                                this->dims());
  }

  ElementArrayView<const value_type>
  valuesView(const Dimensions &dims) const override {
    return makeElementArrayView(m_values.data(), 0, dims, this->dims());
  }
  ElementArrayView<const value_type>
  valuesView(const Dimensions &dims, const Dim dim,
             const scipp::index begin) const override {
    scipp::index beginOffset = this->dims().contains(dim)
                                   ? begin * this->dims().offset(dim)
                                   : begin * this->dims().volume();
    return makeElementArrayView(m_values.data(), beginOffset, dims,
                                this->dims());
  }

  ElementArrayView<value_type> variancesView(const Dimensions &dims) override {
    return makeElementArrayView(m_variances->data(), 0, dims, this->dims());
  }
  ElementArrayView<value_type>
  variancesView(const Dimensions &dims, const Dim dim,
                const scipp::index begin) override {
    scipp::index beginOffset = this->dims().contains(dim)
                                   ? begin * this->dims().offset(dim)
                                   : begin * this->dims().volume();
    return makeElementArrayView(m_variances->data(), beginOffset, dims,
                                this->dims());
  }

  ElementArrayView<const value_type>
  variancesView(const Dimensions &dims) const override {
    return makeElementArrayView(m_variances->data(), 0, dims, this->dims());
  }
  ElementArrayView<const value_type>
  variancesView(const Dimensions &dims, const Dim dim,
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

  bool isSame(const VariableConcept &other) const override {
    return this == &other;
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

/// Implementation of VariableConcept that represents a view onto data.
template <class T>
class ViewModel
    : public VariableConceptT<std::remove_const_t<typename T::element_type>> {
  void requireMutable() const {
    if (isConstView())
      throw std::runtime_error(
          "View is const, cannot get mutable range of data.");
  }
  void requireContiguous() const {
    if (!isContiguous())
      throw std::runtime_error(
          "View is not contiguous, cannot get contiguous range of data.");
  }

public:
  using value_type = typename T::value_type;

  ViewModel(const Dimensions &dimensions, T model,
            std::optional<T> variances = std::nullopt)
      : VariableConceptT<value_type>(std::move(dimensions)),
        m_values(std::move(model)), m_variances(std::move(variances)) {
    if (this->dims().volume() != m_values.size())
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
  }

  scipp::span<value_type> values() override {
    requireMutable();
    requireContiguous();
    if constexpr (std::is_const<typename T::element_type>::value)
      return scipp::span<value_type>();
    else
      return scipp::span(m_values.data(), m_values.data() + size());
  }
  scipp::span<value_type> values(const Dim dim, const scipp::index begin,
                                 const scipp::index end) override {
    requireMutable();
    requireContiguous();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dim);
      static_cast<void>(begin);
      static_cast<void>(end);
      return scipp::span<value_type>();
    } else {
      return makeSpan(m_values, this->dims(), dim, begin, end);
    }
  }

  scipp::span<const value_type> values() const override {
    requireContiguous();
    return scipp::span(m_values.data(), m_values.data() + size());
  }
  scipp::span<const value_type> values(const Dim dim, const scipp::index begin,
                                       const scipp::index end) const override {
    requireContiguous();
    return makeSpan(m_values, this->dims(), dim, begin, end);
  }

  scipp::span<value_type> variances() override {
    requireMutable();
    requireContiguous();
    if constexpr (std::is_const<typename T::element_type>::value)
      return scipp::span<value_type>();
    else
      return scipp::span(m_variances->data(), m_variances->data() + size());
  }
  scipp::span<value_type> variances(const Dim dim, const scipp::index begin,
                                    const scipp::index end) override {
    requireMutable();
    requireContiguous();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dim);
      static_cast<void>(begin);
      static_cast<void>(end);
      return scipp::span<value_type>();
    } else {
      return makeSpan(*m_variances, this->dims(), dim, begin, end);
    }
  }

  scipp::span<const value_type> variances() const override {
    requireContiguous();
    return scipp::span(m_variances->data(), m_variances->data() + size());
  }
  scipp::span<const value_type>
  variances(const Dim dim, const scipp::index begin,
            const scipp::index end) const override {
    requireContiguous();
    return makeSpan(*m_variances, this->dims(), dim, begin, end);
  }

  ElementArrayView<value_type> valuesView(const Dimensions &dims) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dims);
      return ElementArrayView<value_type>(nullptr, 0, {}, {});
    } else {
      return {m_values, dims};
    }
  }
  ElementArrayView<value_type> valuesView(const Dimensions &dims, const Dim dim,
                                          const scipp::index begin) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dim);
      static_cast<void>(begin);
      return ElementArrayView<value_type>(nullptr, 0, {}, {});
    } else {
      return {m_values, dims, dim, begin};
    }
  }

  ElementArrayView<const value_type>
  valuesView(const Dimensions &dims) const override {
    return {m_values, dims};
  }
  ElementArrayView<const value_type>
  valuesView(const Dimensions &dims, const Dim dim,
             const scipp::index begin) const override {
    return {m_values, dims, dim, begin};
  }

  ElementArrayView<value_type> variancesView(const Dimensions &dims) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dims);
      return ElementArrayView<value_type>(nullptr, 0, {}, {});
    } else {
      return {*m_variances, dims};
    }
  }
  ElementArrayView<value_type>
  variancesView(const Dimensions &dims, const Dim dim,
                const scipp::index begin) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dim);
      static_cast<void>(begin);
      return ElementArrayView<value_type>(nullptr, 0, {}, {});
    } else {
      return {*m_variances, dims, dim, begin};
    }
  }

  ElementArrayView<const value_type>
  variancesView(const Dimensions &dims) const override {
    return {*m_variances, dims};
  }
  ElementArrayView<const value_type>
  variancesView(const Dimensions &dims, const Dim dim,
                const scipp::index begin) const override {
    return {*m_variances, dims, dim, begin};
  }

  ElementArrayView<const value_type>
  valuesReshaped(const Dimensions &dims) const override {
    return {m_values, dims};
  }
  ElementArrayView<value_type> valuesReshaped(const Dimensions &dims) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dims);
      return ElementArrayView<value_type>(nullptr, 0, {}, {});
    } else {
      return {m_values, dims};
    }
  }

  ElementArrayView<const value_type>
  variancesReshaped(const Dimensions &dims) const override {
    return {*m_variances, dims};
  }
  ElementArrayView<value_type>
  variancesReshaped(const Dimensions &dims) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dims);
      return ElementArrayView<value_type>(nullptr, 0, {}, {});
    } else {
      return {*m_variances, dims};
    }
  }

  std::unique_ptr<
      VariableConceptT<std::remove_const_t<typename T::element_type>>>
  copyT() const override {
    using DataT =
        detail::element_array<std::remove_const_t<typename T::element_type>>;
    DataT values(m_values.begin(), m_values.end());
    std::optional<DataT> variances;
    if (hasVariances())
      variances = DataT(m_variances->begin(), m_variances->end());
    return std::make_unique<DataModel<DataT>>(this->dims(), std::move(values),
                                              std::move(variances));
  }

  VariableConceptHandle clone() const override {
    return std::make_unique<ViewModel<T>>(this->dims(), m_values, m_variances);
  }

  bool isSame(const VariableConcept &other) const override {
    // Views can be copied but can still refer to same data, so unlike for
    // DataModel a pointer comparison is not sufficient here.
    if (hasVariances() != other.hasVariances())
      return false;
    if (const auto *ptr = dynamic_cast<const ViewModel<T> *>(&other))
      return m_values.isSame(ptr->m_values);
    return false;
  }

  bool isContiguous() const override {
    return this->dims().isContiguousIn(m_values.parentDimensions());
  }
  bool isView() const override { return true; }
  bool isConstView() const override {
    return std::is_const<typename T::element_type>::value;
  }
  bool hasVariances() const noexcept override {
    return m_variances.has_value();
  }

  scipp::index size() const override { return m_values.size(); }

  T m_values;
  std::optional<T> m_variances;

private:
  void setVariances(Variable &&) override {
    throw std::logic_error("This shouldn't be called");
  }
};

template <class T>
Variable::Variable(const units::Unit unit, const Dimensions &dimensions,
                   T object)
    : m_unit{unit}, m_object(std::make_unique<DataModel<T>>(
                        std::move(dimensions), std::move(object))) {}
template <class T>
Variable::Variable(const units::Unit unit, const Dimensions &dimensions,
                   T values_, T variances_)
    : m_unit{unit},
      m_object(variances_.empty()
                   ? std::make_unique<DataModel<T>>(std::move(dimensions),
                                                    std::move(values_))
                   : std::make_unique<DataModel<T>>(std::move(dimensions),
                                                    std::move(values_),
                                                    std::move(variances_))) {}

template <class T>
const detail::element_array<T> &Variable::cast(const bool variances_) const {
  auto &dm = requireT<const DataModel<detail::element_array<T>>>(*m_object);
  if (!variances_)
    return dm.m_values;
  else {
    expect::hasVariances(*this);
    return *dm.m_variances;
  }
}

template <class T>
detail::element_array<T> &Variable::cast(const bool variances_) {
  auto &dm = requireT<DataModel<detail::element_array<T>>>(*m_object);
  if (!variances_)
    return dm.m_values;
  else {
    expect::hasVariances(*this);
    return *dm.m_variances;
  }
}

template <class T>
const ElementArrayView<const T> VariableConstView::cast() const {
  using TT = T;
  if (!m_view)
    return requireT<const DataModel<detail::element_array<TT>>>(data())
        .valuesView(dims());
  if (m_view->isConstView())
    return requireT<const ViewModel<ElementArrayView<const TT>>>(data())
        .m_values;
  // Make a const view from the mutable one.
  return {requireT<const ViewModel<ElementArrayView<TT>>>(data()).m_values,
          dims()};
}

template <class T>
const ElementArrayView<const T> VariableConstView::castVariances() const {
  expect::hasVariances(*this);
  using TT = T;
  if (!m_view)
    return requireT<const DataModel<detail::element_array<TT>>>(data())
        .variancesView(dims());
  if (m_view->isConstView())
    return *requireT<const ViewModel<ElementArrayView<const TT>>>(data())
                .m_variances;
  // Make a const view from the mutable one.
  return {*requireT<const ViewModel<ElementArrayView<TT>>>(data()).m_variances,
          dims()};
}

template <class T> ElementArrayView<T> VariableView::cast() const {
  using TT = T;
  if (m_view)
    return requireT<const ViewModel<ElementArrayView<TT>>>(data()).m_values;
  return requireT<DataModel<detail::element_array<TT>>>(data()).valuesView(
      dims());
}

template <class T> ElementArrayView<T> VariableView::castVariances() const {
  expect::hasVariances(*this);
  using TT = T;
  if (m_view)
    return *requireT<const ViewModel<ElementArrayView<TT>>>(data()).m_variances;
  return requireT<DataModel<detail::element_array<TT>>>(data()).variancesView(
      dims());
}

/**
  Support explicit instantiations for templates for generic Variable and
  VariableConstView
*/
using scipp::core::detail::element_array;
#define INSTANTIATE_VARIABLE(...)                                              \
  template Variable::Variable(const units::Unit, const Dimensions &,           \
                              element_array<__VA_ARGS__>);                     \
  template Variable::Variable(const units::Unit, const Dimensions &,           \
                              element_array<__VA_ARGS__>,                      \
                              element_array<__VA_ARGS__>);                     \
  template element_array<__VA_ARGS__> &Variable::cast<__VA_ARGS__>(            \
      const bool);                                                             \
  template const element_array<__VA_ARGS__> &Variable::cast<__VA_ARGS__>(      \
      const bool) const;                                                       \
  template const ElementArrayView<const __VA_ARGS__>                           \
  VariableConstView::cast<__VA_ARGS__>() const;                               \
  template const ElementArrayView<const __VA_ARGS__>                           \
  VariableConstView::castVariances<__VA_ARGS__>() const;                      \
  template ElementArrayView<__VA_ARGS__> VariableView::cast<__VA_ARGS__>()    \
      const;                                                                   \
  template ElementArrayView<__VA_ARGS__>                                       \
  VariableView::castVariances<__VA_ARGS__>() const;

} // namespace scipp::core
