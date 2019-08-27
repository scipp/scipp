// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold

#include "scipp/core/apply.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/variable.h"
#include "scipp/core/variable_view.h"
#include "scipp/core/vector.h"
#include "scipp/units/unit.h"

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
template <class T>
VariableConceptHandle VariableConceptT<T>::makeDefaultFromParent(const Dimensions &dims) const {
  using TT = Vector<std::decay_t<T>>;
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
    if (m_variances && !canHaveVariances<value_type>())
      throw except::VariancesError("This data type cannot have variances.");
    if (this->dims().volume() != scipp::size(m_values))
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
  }

  void setVariances(Vector<value_type> &&v) override {
    if (!canHaveVariances<value_type>())
      throw except::VariancesError("This data type cannot have variances.");
    if (v.size() != m_values.size())
      throw except::SizeError(std::string("Sizes should match in ") +
                              __PRETTY_FUNCTION__);
    m_variances.emplace(std::move(v));
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
  void setVariances(Vector<value_type> &&) override {
    throw std::logic_error(std::string("This shouldn't be called: ") +
                           __PRETTY_FUNCTION__);
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

template <class T> void Variable::setVariances(Vector<T> &&v) {
  auto lmb = [v = std::forward<decltype(v)>(v)](auto &&model) mutable {
    using TT = underlying_type_t<T>;
    // Handle, e.g., float input if Variable dtype is double.
    using TTT = typename std::decay_t<decltype(*model)>::value_type;
    if constexpr (std::is_same_v<TTT, T> && std::is_same_v<T, TT>)
      model->setVariances(std::move(v));
    else
      model->setVariances(Vector<TTT>(v.begin(), v.end()));
  };
  try {
    apply_in_place<double, float, int64_t, int32_t>(lmb, *this);
  } catch (std::bad_variant_access &e) {
    throw except::TypeError(std::string("Can't set variance for the type: ") +
                            to_string(dtype()));
  }
}

template <class T> void VariableProxy::setVariances(Vector<T> &&v) const {
  // If the proxy wraps the whole variable (common case: iterating a dataset)
  // m_view is not set. A more complex check would be to verify dimensions,
  // shape, and strides, but this should be sufficient for now.
  if (m_view)
    throw except::VariancesError(
        "Cannot add variances via sliced or reshaped view of Variable.");
  m_mutableVariable->setVariances(std::move(v));
}

/**
  Support explicit instantiations for templates for generic Variable and
  VariableConstProxy
*/
#define INSTANTIATE_VARIABLE(...)                                              \
  template Variable::Variable(const units::Unit, const Dimensions &,           \
                              Vector<underlying_type_t<__VA_ARGS__>>);         \
  template Variable::Variable(const units::Unit, const Dimensions &,           \
                              Vector<underlying_type_t<__VA_ARGS__>>,          \
                              Vector<underlying_type_t<__VA_ARGS__>>);         \
  template Vector<underlying_type_t<__VA_ARGS__>>                              \
      &Variable::cast<__VA_ARGS__>(const bool);                                \
  template const Vector<underlying_type_t<__VA_ARGS__>>                        \
      &Variable::cast<__VA_ARGS__>(const bool) const;                          \
  template const VariableView<const underlying_type_t<__VA_ARGS__>>            \
  VariableConstProxy::cast<__VA_ARGS__>() const;                               \
  template const VariableView<const underlying_type_t<__VA_ARGS__>>            \
  VariableConstProxy::castVariances<__VA_ARGS__>() const;                      \
  template VariableView<underlying_type_t<__VA_ARGS__>>                        \
  VariableProxy::cast<__VA_ARGS__>() const;                                    \
  template VariableView<underlying_type_t<__VA_ARGS__>>                        \
  VariableProxy::castVariances<__VA_ARGS__>() const;

#define INSTANTIATE_SET_VARIANCES(...)                                         \
  template void Variable::setVariances<__VA_ARGS__>(Vector<__VA_ARGS__> &&);   \
  template void VariableProxy::setVariances<__VA_ARGS__>(                      \
      Vector<__VA_ARGS__> &&) const;

} // namespace scipp::core
