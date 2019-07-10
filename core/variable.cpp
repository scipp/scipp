// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>
#include <string>

#include "scipp/core/apply.h"
#include "scipp/core/counts.h"
#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"
#include "scipp/core/variable_view.h"

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

template <class T> class DataModel;
template <class T> class VariableConceptT;
template <class T> struct RebinHelper {
  // Special rebin version for rebinning inner dimension to a joint new coord.
  static void rebinInner(const Dim dim, const VariableConceptT<T> &oldT,
                         VariableConceptT<T> &newT,
                         const VariableConceptT<T> &oldCoordT,
                         const VariableConceptT<T> &newCoordT) {
    const auto &oldData = oldT.values();
    auto newData = newT.values();
    const auto oldSize = oldT.dims()[dim];
    const auto newSize = newT.dims()[dim];
    const auto count = oldT.dims().volume() / oldSize;
    const auto *xold = &*oldCoordT.values().begin();
    const auto *xnew = &*newCoordT.values().begin();
    // This function assumes that dimensions between coord and data either
    // match, or coord is 1D.
    const bool jointOld = oldCoordT.dims().shape().size() == 1;
    const bool jointNew = newCoordT.dims().shape().size() == 1;
#pragma omp parallel for
    for (scipp::index c = 0; c < count; ++c) {
      scipp::index iold = 0;
      scipp::index inew = 0;
      const scipp::index oldEdgeOffset = jointOld ? 0 : c * (oldSize + 1);
      const scipp::index newEdgeOffset = jointNew ? 0 : c * (newSize + 1);
      const auto oldOffset = c * oldSize;
      const auto newOffset = c * newSize;
      while ((iold < oldSize) && (inew < newSize)) {
        auto xo_low = xold[oldEdgeOffset + iold];
        auto xo_high = xold[oldEdgeOffset + iold + 1];
        auto xn_low = xnew[newEdgeOffset + inew];
        auto xn_high = xnew[newEdgeOffset + inew + 1];

        if (xn_high <= xo_low)
          inew++; /* old and new bins do not overlap */
        else if (xo_high <= xn_low)
          iold++; /* old and new bins do not overlap */
        else {
          // delta is the overlap of the bins on the x axis
          auto delta = xo_high < xn_high ? xo_high : xn_high;
          delta -= xo_low > xn_low ? xo_low : xn_low;

          auto owidth = xo_high - xo_low;
          newData[newOffset + inew] +=
              oldData[oldOffset + iold] * delta / owidth;

          if (xn_high > xo_high) {
            iold++;
          } else {
            inew++;
          }
        }
      }
    }
  }
};

template <typename T> struct RebinGeneralHelper {
  static void rebin(const Dim dim, const Variable &oldT, Variable &newT,
                    const Variable &oldCoordT, const Variable &newCoordT) {
    const auto oldSize = oldT.dims()[dim];
    const auto newSize = newT.dims()[dim];

    const auto *xold = oldCoordT.values<T>().data();
    const auto *xnew = newCoordT.values<T>().data();
    // This function assumes that dimensions between coord and data
    // coord is 1D.
    int iold = 0;
    int inew = 0;
    while ((iold < oldSize) && (inew < newSize)) {
      auto xo_low = xold[iold];
      auto xo_high = xold[iold + 1];
      auto xn_low = xnew[inew];
      auto xn_high = xnew[inew + 1];

      if (xn_high <= xo_low)
        inew++; /* old and new bins do not overlap */
      else if (xo_high <= xn_low)
        iold++; /* old and new bins do not overlap */
      else {
        // delta is the overlap of the bins on the x axis
        auto delta = xo_high < xn_high ? xo_high : xn_high;
        delta -= xo_low > xn_low ? xo_low : xn_low;

        auto owidth = xo_high - xo_low;
        newT.slice({dim, inew}) += oldT.slice({dim, iold}) * delta / owidth;
        if (xn_high > xo_high) {
          iold++;
        } else {
          inew++;
        }
      }
    }
  }
};

template <class T> class ViewModel;

VariableConcept::VariableConcept(const Dimensions &dimensions)
    : m_dimensions(dimensions) {}

bool isMatchingOr1DBinEdge(const Dim dim, Dimensions edges,
                           const Dimensions &toMatch) {
  if (edges.shape().size() == 1)
    return true;
  edges.resize(dim, edges[dim] - 1);
  return edges == toMatch;
}

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
  // Four cases for minimizing use of VariableView --- just copy contiguous
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
    if (this->dims().volume() != scipp::size(m_values))
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
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

  VariableView<value_type> valuesView(const Dimensions &dims) override {
    return makeVariableView(m_values.data(), 0, dims, this->dims());
  }
  VariableView<value_type> valuesView(const Dimensions &dims, const Dim dim,
                                      const scipp::index begin) override {
    scipp::index beginOffset = this->dims().contains(dim)
                                   ? begin * this->dims().offset(dim)
                                   : begin * this->dims().volume();
    return makeVariableView(m_values.data(), beginOffset, dims, this->dims());
  }

  VariableView<const value_type>
  valuesView(const Dimensions &dims) const override {
    return makeVariableView(m_values.data(), 0, dims, this->dims());
  }
  VariableView<const value_type>
  valuesView(const Dimensions &dims, const Dim dim,
             const scipp::index begin) const override {
    scipp::index beginOffset = this->dims().contains(dim)
                                   ? begin * this->dims().offset(dim)
                                   : begin * this->dims().volume();
    return makeVariableView(m_values.data(), beginOffset, dims, this->dims());
  }

  VariableView<value_type> variancesView(const Dimensions &dims) override {
    return makeVariableView(m_variances->data(), 0, dims, this->dims());
  }
  VariableView<value_type> variancesView(const Dimensions &dims, const Dim dim,
                                         const scipp::index begin) override {
    scipp::index beginOffset = this->dims().contains(dim)
                                   ? begin * this->dims().offset(dim)
                                   : begin * this->dims().volume();
    return makeVariableView(m_variances->data(), beginOffset, dims,
                            this->dims());
  }

  VariableView<const value_type>
  variancesView(const Dimensions &dims) const override {
    return makeVariableView(m_variances->data(), 0, dims, this->dims());
  }
  VariableView<const value_type>
  variancesView(const Dimensions &dims, const Dim dim,
                const scipp::index begin) const override {
    scipp::index beginOffset = this->dims().contains(dim)
                                   ? begin * this->dims().offset(dim)
                                   : begin * this->dims().volume();
    return makeVariableView(m_variances->data(), beginOffset, dims,
                            this->dims());
  }

  VariableView<const value_type>
  valuesReshaped(const Dimensions &dims) const override {
    return makeVariableView(m_values.data(), 0, dims, dims);
  }
  VariableView<value_type> valuesReshaped(const Dimensions &dims) override {
    return makeVariableView(m_values.data(), 0, dims, dims);
  }

  VariableView<const value_type>
  variancesReshaped(const Dimensions &dims) const override {
    return makeVariableView(m_variances->data(), 0, dims, dims);
  }
  VariableView<value_type> variancesReshaped(const Dimensions &dims) override {
    return makeVariableView(m_variances->data(), 0, dims, dims);
  }

  std::unique_ptr<VariableConceptT<typename T::value_type>>
  copyT() const override {
    return std::make_unique<DataModel<T>>(*this);
  }

  VariableConceptHandle clone() const override {
    return std::make_unique<DataModel<T>>(this->dims(), m_values, m_variances);
  }

  VariableConceptHandle clone(const Dimensions &dims) const override {
    if (hasVariances())
      return std::make_unique<DataModel<T>>(dims, T(dims.volume()),
                                            T(dims.volume()));
    else
      return std::make_unique<DataModel<T>>(dims, T(dims.volume()));
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

  VariableView<value_type> valuesView(const Dimensions &dims) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dims);
      return VariableView<value_type>(nullptr, 0, {}, {});
    } else {
      return {m_values, dims};
    }
  }
  VariableView<value_type> valuesView(const Dimensions &dims, const Dim dim,
                                      const scipp::index begin) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dim);
      static_cast<void>(begin);
      return VariableView<value_type>(nullptr, 0, {}, {});
    } else {
      return {m_values, dims, dim, begin};
    }
  }

  VariableView<const value_type>
  valuesView(const Dimensions &dims) const override {
    return {m_values, dims};
  }
  VariableView<const value_type>
  valuesView(const Dimensions &dims, const Dim dim,
             const scipp::index begin) const override {
    return {m_values, dims, dim, begin};
  }

  VariableView<value_type> variancesView(const Dimensions &dims) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dims);
      return VariableView<value_type>(nullptr, 0, {}, {});
    } else {
      return {*m_variances, dims};
    }
  }
  VariableView<value_type> variancesView(const Dimensions &dims, const Dim dim,
                                         const scipp::index begin) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dim);
      static_cast<void>(begin);
      return VariableView<value_type>(nullptr, 0, {}, {});
    } else {
      return {*m_variances, dims, dim, begin};
    }
  }

  VariableView<const value_type>
  variancesView(const Dimensions &dims) const override {
    return {*m_variances, dims};
  }
  VariableView<const value_type>
  variancesView(const Dimensions &dims, const Dim dim,
                const scipp::index begin) const override {
    return {*m_variances, dims, dim, begin};
  }

  VariableView<const value_type>
  valuesReshaped(const Dimensions &dims) const override {
    return {m_values, dims};
  }
  VariableView<value_type> valuesReshaped(const Dimensions &dims) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dims);
      return VariableView<value_type>(nullptr, 0, {}, {});
    } else {
      return {m_values, dims};
    }
  }

  VariableView<const value_type>
  variancesReshaped(const Dimensions &dims) const override {
    return {*m_variances, dims};
  }
  VariableView<value_type> variancesReshaped(const Dimensions &dims) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dims);
      return VariableView<value_type>(nullptr, 0, {}, {});
    } else {
      return {*m_variances, dims};
    }
  }

  std::unique_ptr<
      VariableConceptT<std::remove_const_t<typename T::element_type>>>
  copyT() const override {
    using DataT = Vector<std::remove_const_t<typename T::element_type>>;
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

  VariableConceptHandle clone(const Dimensions &) const override {
    throw std::runtime_error("Cannot resize view.");
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
};

Variable::Variable(const VariableConstProxy &slice)
    : Variable(*slice.m_variable) {
  if (slice.m_view) {
    setUnit(slice.unit());
    setDims(slice.dims());
    // There is a bug in the implementation of MultiIndex used in VariableView
    // in case one of the dimensions has extent 0.
    if (dims().volume() != 0)
      data().copy(slice.data(), Dim::Invalid, 0, 0, 1);
  }
}
Variable::Variable(const Variable &parent, const Dimensions &dims)
    : m_unit(parent.unit()), m_object(parent.m_object->clone(dims)) {}

Variable::Variable(const VariableConstProxy &parent, const Dimensions &dims)
    : m_unit(parent.unit()), m_object(parent.data().clone(dims)) {}

Variable::Variable(const Variable &parent, VariableConceptHandle data)
    : m_unit(parent.unit()), m_object(std::move(data)) {}

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

void Variable::setDims(const Dimensions &dimensions) {
  if (dimensions.volume() == m_object->dims().volume()) {
    if (dimensions != m_object->dims())
      data().m_dimensions = dimensions;
    return;
  }
  m_object = m_object->clone(dimensions);
}

template <class T>
const Vector<underlying_type_t<T>> &
Variable::cast(const bool variances_) const {
  auto &dm = requireT<const DataModel<Vector<underlying_type_t<T>>>>(*m_object);
  if (!variances_)
    return dm.m_values;
  else {
    if (!hasVariances())
      throw std::runtime_error("No variances");
    return *dm.m_variances;
  }
}

template <class T>
Vector<underlying_type_t<T>> &Variable::cast(const bool variances_) {
  auto &dm = requireT<DataModel<Vector<underlying_type_t<T>>>>(*m_object);
  if (!variances_)
    return dm.m_values;
  else {
    if (!hasVariances())
      throw std::runtime_error("No variances");
    return *dm.m_variances;
  }
}

#define INSTANTIATE(...)                                                       \
  template Variable::Variable(const units::Unit, const Dimensions &,           \
                              Vector<underlying_type_t<__VA_ARGS__>>);         \
  template Variable::Variable(const units::Unit, const Dimensions &,           \
                              Vector<underlying_type_t<__VA_ARGS__>>,          \
                              Vector<underlying_type_t<__VA_ARGS__>>);         \
  template Vector<underlying_type_t<__VA_ARGS__>>                              \
      &Variable::cast<__VA_ARGS__>(const bool);                                \
  template const Vector<underlying_type_t<__VA_ARGS__>>                        \
      &Variable::cast<__VA_ARGS__>(const bool) const;

INSTANTIATE(std::string)
INSTANTIATE(double)
INSTANTIATE(float)
INSTANTIATE(int64_t)
INSTANTIATE(int32_t)
INSTANTIATE(bool)
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
INSTANTIATE(scipp::index)
#endif
INSTANTIATE(Dataset)
INSTANTIATE(Eigen::Vector3d)
INSTANTIATE(sparse_container<double>)
INSTANTIATE(sparse_container<float>)
INSTANTIATE(sparse_container<int64_t>)
INSTANTIATE(sparse_container<int32_t>)
// Some sparse instantiations are only needed to avoid linker errors: Some
// makeVariable overloads have a runtime branch that may instantiate a sparse
// variable.
INSTANTIATE(sparse_container<std::string>)
INSTANTIATE(sparse_container<Bool>)
INSTANTIATE(sparse_container<Dataset>)
INSTANTIATE(sparse_container<Eigen::Vector3d>)

template <class T1, class T2> bool equals(const T1 &a, const T2 &b) {
  if (!a || !b)
    return static_cast<bool>(a) == static_cast<bool>(b);
  if (a.unit() != b.unit())
    return false;
  return a.data() == b.data();
}

bool Variable::operator==(const Variable &other) const {
  return equals(*this, other);
}

bool Variable::operator==(const VariableConstProxy &other) const {
  return equals(*this, other);
}

bool Variable::operator!=(const Variable &other) const {
  return !(*this == other);
}

bool Variable::operator!=(const VariableConstProxy &other) const {
  return !(*this == other);
}

template <class T> VariableProxy VariableProxy::assign(const T &other) const {
  setUnit(other.unit());
  if (dims() != other.dims())
    throw except::DimensionMismatchError(dims(), other.dims());
  data().copy(other.data(), Dim::Invalid, 0, 0, 1);
  return *this;
}

template VariableProxy VariableProxy::assign(const Variable &) const;
template VariableProxy VariableProxy::assign(const VariableConstProxy &) const;

bool VariableConstProxy::operator==(const Variable &other) const {
  // Always use deep comparison (pointer comparison does not make sense since we
  // may be looking at a different section).
  return equals(*this, other);
}
bool VariableConstProxy::operator==(const VariableConstProxy &other) const {
  return equals(*this, other);
}

bool VariableConstProxy::operator!=(const Variable &other) const {
  return !(*this == other);
}
bool VariableConstProxy::operator!=(const VariableConstProxy &other) const {
  return !(*this == other);
}

void VariableProxy::setUnit(const units::Unit &unit) const {
  expectCanSetUnit(unit);
  m_mutableVariable->setUnit(unit);
}

void VariableProxy::expectCanSetUnit(const units::Unit &unit) const {
  if ((this->unit() != unit) && (dims() != m_mutableVariable->dims()))
    throw except::UnitError("Partial view on data of variable cannot be used "
                            "to change the unit.");
}

template <class T>
const VariableView<const underlying_type_t<T>>
VariableConstProxy::cast() const {
  using TT = underlying_type_t<T>;
  if (!m_view)
    return requireT<const DataModel<Vector<TT>>>(data()).valuesView(dims());
  if (m_view->isConstView())
    return requireT<const ViewModel<VariableView<const TT>>>(data()).m_values;
  // Make a const view from the mutable one.
  return {requireT<const ViewModel<VariableView<TT>>>(data()).m_values, dims()};
}

template <class T>
const VariableView<const underlying_type_t<T>>
VariableConstProxy::castVariances() const {
  using TT = underlying_type_t<T>;
  if (!m_view)
    return requireT<const DataModel<Vector<TT>>>(data()).variancesView(dims());
  if (m_view->isConstView())
    return *requireT<const ViewModel<VariableView<const TT>>>(data())
                .m_variances;
  // Make a const view from the mutable one.
  return {*requireT<const ViewModel<VariableView<TT>>>(data()).m_variances,
          dims()};
}

template <class T>
VariableView<underlying_type_t<T>> VariableProxy::cast() const {
  using TT = underlying_type_t<T>;
  if (m_view)
    return requireT<const ViewModel<VariableView<TT>>>(data()).m_values;
  return requireT<DataModel<Vector<TT>>>(data()).valuesView(dims());
}

template <class T>
VariableView<underlying_type_t<T>> VariableProxy::castVariances() const {
  using TT = underlying_type_t<T>;
  if (m_view)
    return *requireT<const ViewModel<VariableView<TT>>>(data()).m_variances;
  return requireT<DataModel<Vector<TT>>>(data()).variancesView(dims());
}

#define INSTANTIATE_SLICEVIEW(...)                                             \
  template const VariableView<const underlying_type_t<__VA_ARGS__>>            \
  VariableConstProxy::cast<__VA_ARGS__>() const;                               \
  template const VariableView<const underlying_type_t<__VA_ARGS__>>            \
  VariableConstProxy::castVariances<__VA_ARGS__>() const;                      \
  template VariableView<underlying_type_t<__VA_ARGS__>>                        \
  VariableProxy::cast<__VA_ARGS__>() const;                                    \
  template VariableView<underlying_type_t<__VA_ARGS__>>                        \
  VariableProxy::castVariances<__VA_ARGS__>() const;

INSTANTIATE_SLICEVIEW(double)
INSTANTIATE_SLICEVIEW(float)
INSTANTIATE_SLICEVIEW(int64_t)
INSTANTIATE_SLICEVIEW(int32_t)
INSTANTIATE_SLICEVIEW(bool)
INSTANTIATE_SLICEVIEW(std::string)
INSTANTIATE_SLICEVIEW(boost::container::small_vector<double, 8>)
INSTANTIATE_SLICEVIEW(Dataset)
INSTANTIATE_SLICEVIEW(Eigen::Vector3d)

VariableConstProxy Variable::slice(const Slice slice) const & {
  return {*this, slice.dim, slice.begin, slice.end};
}

Variable Variable::slice(const Slice slice) const && {
  return {this->slice(slice)};
}

VariableProxy Variable::slice(const Slice slice) & {
  return {*this, slice.dim, slice.begin, slice.end};
}

Variable Variable::slice(const Slice slice) && { return {this->slice(slice)}; }

VariableConstProxy Variable::reshape(const Dimensions &dims) const & {
  return {*this, dims};
}

VariableProxy Variable::reshape(const Dimensions &dims) & {
  return {*this, dims};
}

Variable Variable::reshape(const Dimensions &dims) && {
  Variable reshaped(std::move(*this));
  reshaped.setDims(dims);
  return reshaped;
}

Variable VariableConstProxy::reshape(const Dimensions &dims) const {
  // In general a variable slice is not contiguous. Therefore we cannot reshape
  // without making a copy (except for special cases).
  Variable reshaped(*this);
  reshaped.setDims(dims);
  return reshaped;
}

// Example of a "derived" operation: Implementation does not require adding a
// virtual function to VariableConcept.
std::vector<Variable> split(const Variable &var, const Dim dim,
                            const std::vector<scipp::index> &indices) {
  if (indices.empty())
    return {var};
  std::vector<Variable> vars;
  vars.emplace_back(var.slice({dim, 0, indices.front()}));
  for (scipp::index i = 0; i < scipp::size(indices) - 1; ++i)
    vars.emplace_back(var.slice({dim, indices[i], indices[i + 1]}));
  vars.emplace_back(var.slice({dim, indices.back(), var.dims()[dim]}));
  return vars;
}

Variable concatenate(const Variable &a1, const Variable &a2, const Dim dim) {
  if (a1.dtype() != a2.dtype())
    throw std::runtime_error(
        "Cannot concatenate Variables: Data types do not match.");
  if (a1.unit() != a2.unit())
    throw std::runtime_error(
        "Cannot concatenate Variables: Units do not match.");

  if (a1.dims().sparseDim() == dim && a2.dims().sparseDim() == dim) {
    Variable out(a1);
    transform_in_place<pair_self_t<sparse_container<double>>>(
        out, a2,
        [](auto &a, const auto &b) { a.insert(a.end(), b.begin(), b.end()); });
    return out;
  }

  const auto &dims1 = a1.dims();
  const auto &dims2 = a2.dims();
  // TODO Many things in this function should be refactored and moved in class
  // Dimensions.
  // TODO Special handling for edge variables.
  if (dims1.sparseDim() != dims2.sparseDim())
    throw std::runtime_error("Cannot concatenate Variables: Either both or "
                             "neither must be sparse, and the sparse "
                             "dimensions must be the same.");
  for (const auto &dim1 : dims1.denseLabels()) {
    if (dim1 != dim) {
      if (!dims2.contains(dim1))
        throw std::runtime_error(
            "Cannot concatenate Variables: Dimensions do not match.");
      if (dims2[dim1] != dims1[dim1])
        throw std::runtime_error(
            "Cannot concatenate Variables: Dimension extents do not match.");
    }
  }
  auto size1 = dims1.shape().size();
  auto size2 = dims2.shape().size();
  if (dims1.contains(dim))
    size1--;
  if (dims2.contains(dim))
    size2--;
  // This check covers the case of dims2 having extra dimensions not present in
  // dims1.
  // TODO Support broadcast of dimensions?
  if (size1 != size2)
    throw std::runtime_error(
        "Cannot concatenate Variables: Dimensions do not match.");

  auto out(a1);
  auto dims(dims1);
  scipp::index extent1 = 1;
  scipp::index extent2 = 1;
  if (dims1.contains(dim))
    extent1 += dims1[dim] - 1;
  if (dims2.contains(dim))
    extent2 += dims2[dim] - 1;
  if (dims.contains(dim))
    dims.resize(dim, extent1 + extent2);
  else
    dims.add(dim, extent1 + extent2);
  out.setDims(dims);

  out.data().copy(a1.data(), dim, 0, 0, extent1);
  out.data().copy(a2.data(), dim, extent1, 0, extent2);

  return out;
}

Variable rebin(const Variable &var, const Variable &oldCoord,
               const Variable &newCoord) {
// TODO Disabled since it is using neutron-specific units. Should be moved
// into scipp-neutron? On the other hand, counts is actually more generic than
// neutron data, but requiring this unit to be part of all supported unit
// systems does not make sense either, I think.
#ifndef SCIPP_UNITS_NEUTRON
  throw std::runtime_error("rebin is disabled for this set of units");
#else
  expect::countsOrCountsDensity(var);
  Dim dim = Dim::Invalid;
  for (const auto d : oldCoord.dims().labels())
    if (oldCoord.dims()[d] == var.dims()[d] + 1) {
      dim = d;
      break;
    }

  auto do_rebin = [dim](auto &&out, auto &&old, auto &&oldCoord_,
                        auto &&newCoord_) {
    // Dimensions of *this and old are guaranteed to be the same.
    const auto &oldT = *old;
    const auto &oldCoordT = *oldCoord_;
    const auto &newCoordT = *newCoord_;
    auto &outT = *out;
    const auto &dims = outT.dims();
    if (dims.inner() == dim &&
        isMatchingOr1DBinEdge(dim, oldCoordT.dims(), oldT.dims()) &&
        isMatchingOr1DBinEdge(dim, newCoordT.dims(), dims)) {
      RebinHelper<typename std::remove_reference_t<decltype(
          outT)>::value_type>::rebinInner(dim, oldT, outT, oldCoordT,
                                          newCoordT);
    } else {
      throw std::runtime_error(
          "TODO the new coord should be 1D or the same dim as newCoord.");
    }
  };

  if (var.unit() == units::counts) {
    auto dims = var.dims();
    dims.resize(dim, newCoord.dims()[dim] - 1);
    Variable rebinned(var, dims);
    if (rebinned.dims().inner() == dim) {
      apply_in_place<double, float>(do_rebin, rebinned, var, oldCoord,
                                    newCoord);
    } else {
      if (newCoord.dims().shape().size() > 1)
        throw std::runtime_error(
            "Not inner rebin works only for 1d coordinates for now.");
      switch (rebinned.dtype()) {
      case dtype<double>:
        RebinGeneralHelper<double>::rebin(dim, var, rebinned, oldCoord,
                                          newCoord);
        break;
      case dtype<float>:
        RebinGeneralHelper<float>::rebin(dim, var, rebinned, oldCoord,
                                         newCoord);
        break;
      default:
        throw std::runtime_error(
            "Rebinning is possible only for double and float types.");
      }
    }
    return rebinned;
  } else {
    // TODO This will currently fail if the data is a multi-dimensional density.
    // Would need a conversion that converts only the rebinned dimension.
    // TODO This could be done more efficiently without a temporary Dataset.
    throw std::runtime_error("Temporarily disabled for refactor");
    /*
    Dataset density;
    density.insert(dimensionCoord(dim), oldCoord);
    density.insert(Data::Value, var);
    auto cnts = counts::fromDensity(std::move(density), dim).erase(Data::Value);
    Dataset rebinnedCounts;
    rebinnedCounts.insert(dimensionCoord(dim), newCoord);
    rebinnedCounts.insert(Data::Value,
                          rebin(std::get<Variable>(cnts), oldCoord, newCoord));
    return std::get<Variable>(
        counts::toDensity(std::move(rebinnedCounts), dim).erase(Data::Value));
    */
  }
#endif
}

Variable permute(const Variable &var, const Dim dim,
                 const std::vector<scipp::index> &indices) {
  auto permuted(var);
  for (size_t i = 0; i < indices.size(); ++i)
    permuted.data().copy(var.data(), dim, i, indices[i], indices[i] + 1);
  return permuted;
}

Variable filter(const Variable &var, const Variable &filter) {
  if (filter.dims().shape().size() != 1)
    throw std::runtime_error(
        "Cannot filter variable: The filter must by 1-dimensional.");
  const auto dim = filter.dims().labels()[0];
  auto mask = filter.values<bool>();

  const scipp::index removed = std::count(mask.begin(), mask.end(), 0);
  if (removed == 0)
    return var;

  auto out(var);
  auto dims = out.dims();
  dims.resize(dim, dims[dim] - removed);
  out.setDims(dims);

  scipp::index iOut = 0;
  // Note: Could copy larger chunks of applicable for better(?) performance.
  // Note: This implementation is inefficient, since we need to cast to concrete
  // type for *every* slice. Should be combined into a single virtual call.
  for (scipp::index iIn = 0; iIn < mask.size(); ++iIn)
    if (mask[iIn])
      out.data().copy(var.data(), dim, iOut++, iIn, iIn + 1);
  return out;
}

Variable sum(const Variable &var, const Dim dim) {
  auto summed(var);
  auto dims = summed.dims();
  dims.erase(dim);
  // setDims zeros the data
  summed.setDims(dims);
  transform_in_place<pair_self_t<double, float, int64_t, Eigen::Vector3d>>(
      summed, var, [](auto &&a, auto &&b) { a += b; });
  return summed;
}

Variable mean(const Variable &var, const Dim dim) {
  auto summed = sum(var, dim);
  double scale = 1.0 / static_cast<double>(var.dims()[dim]);
  return summed * makeVariable<double>(scale);
}

Variable abs(const Variable &var) {
  using std::abs;
  return transform<double, float>(var, [](const auto x) { return abs(x); });
}

Variable norm(const Variable &var) {
  return transform<Eigen::Vector3d>(var, [](auto &&x) { return x.norm(); });
}

Variable sqrt(const Variable &var) {
  using std::sqrt;
  Variable result =
      transform<double, float>(var, [](const auto x) { return sqrt(x); });
  result.setUnit(sqrt(var.unit()));
  return result;
}

Variable broadcast(Variable var, const Dimensions &dims) {
  if (var.dims().contains(dims))
    return var;
  auto newDims = var.dims();
  const auto labels = dims.labels();
  for (auto it = labels.end(); it != labels.begin();) {
    --it;
    const auto label = *it;
    if (newDims.contains(label))
      expect::dimensionMatches(newDims, label, dims[label]);
    else
      newDims.add(label, dims[label]);
  }
  Variable result(var);
  result.setDims(newDims);
  result.data().copy(var.data(), Dim::Invalid, 0, 0, 1);
  return result;
}

void swap(Variable &var, const Dim dim, const scipp::index a,
          const scipp::index b) {
  const Variable tmp = var.slice({dim, a});
  var.slice({dim, a}).assign(var.slice({dim, b}));
  var.slice({dim, b}).assign(tmp);
}

Variable reverse(Variable var, const Dim dim) {
  const auto size = var.dims()[dim];
  for (scipp::index i = 0; i < size / 2; ++i)
    swap(var, dim, i, size - i - 1);
  return var;
}

template <>
VariableView<const double> getView<double>(const Variable &var,
                                           const Dimensions &dims) {
  return requireT<const VariableConceptT<double>>(var.data()).valuesView(dims);
}

} // namespace scipp::core
