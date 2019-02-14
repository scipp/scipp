/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "variable.h"
#include "dataset.h"
#include "except.h"
#include "variable_view.h"

template <template <class> class Op, class T> struct ArithmeticHelper {
  template <class InputView, class OutputView>
  static void apply(const OutputView &a, const InputView &b) {
    std::transform(a.begin(), a.end(), b.begin(), a.begin(), Op<T>());
  }
};

template <class T1, class T2> bool equal(const T1 &view1, const T2 &view2) {
  return std::equal(view1.begin(), view1.end(), view2.begin(), view2.end());
}

template <class T> class DataModel;
template <class T> class VariableConceptT;
template <class T> struct RebinHelper {
  static void rebin(const Dim dim, const gsl::span<const T> &oldModel,
                    const gsl::span<T> &newModel,
                    const VariableView<const T> &oldCoordView,
                    const gsl::index oldOffset,
                    const VariableView<const T> &newCoordView,
                    const gsl::index newOffset) {
    // Why is this not used. Bug?
    static_cast<void>(dim);

    auto oldCoordIt = oldCoordView.begin();
    auto newCoordIt = newCoordView.begin();
    auto oldIt = oldModel.begin();
    auto newIt = newModel.begin();
    while (newIt != newModel.end() && oldIt != oldModel.end()) {
      if (&*(oldCoordIt + 1) == &(*oldCoordIt) + oldOffset) {
        // Last bin in this 1D subhistogram, go to next.
        ++oldCoordIt;
        ++oldIt;
        continue;
      }
      const auto xo_low = *oldCoordIt;
      const auto xo_high = *(&(*oldCoordIt) + oldOffset);
      if (&*(newCoordIt + 1) == &(*newCoordIt) + newOffset) {
        // Last bin in this 1D subhistogram, go to next.
        ++newCoordIt;
        ++newIt;
        continue;
      }
      const auto xn_low = *newCoordIt;
      const auto xn_high = *(&(*newCoordIt) + newOffset);
      if (xn_high <= xo_low) {
        // No overlap, go to next new bin
        ++newCoordIt;
        ++newIt;
      } else if (xo_high <= xn_low) {
        // No overlap, go to next old bin
        ++oldCoordIt;
        ++oldIt;
      } else {
        auto delta = xo_high < xn_high ? xo_high : xn_high;
        delta -= xo_low > xn_low ? xo_low : xn_low;
        *newIt += *oldIt * delta / (xo_high - xo_low);

        if (xn_high > xo_high) {
          ++oldCoordIt;
          ++oldIt;
        } else {
          ++newCoordIt;
          ++newIt;
        }
      }
    }
  }

  // Special rebin version for rebinning inner dimension to a joint new coord.
  static void rebinInner(const Dim dim, const VariableConceptT<T> &oldT,
                         VariableConceptT<T> &newT,
                         const VariableConceptT<T> &oldCoordT,
                         const VariableConceptT<T> &newCoordT) {
    const auto &oldData = oldT.getSpan();
    auto newData = newT.getSpan();
    const auto oldSize = oldT.dimensions()[dim];
    const auto newSize = newT.dimensions()[dim];
    const auto count = oldT.dimensions().volume() / oldSize;
    const auto *xold = &*oldCoordT.getSpan().begin();
    const auto *xnew = &*newCoordT.getSpan().begin();
#pragma omp parallel for
    for (gsl::index c = 0; c < count; ++c) {
      gsl::index iold = 0;
      gsl::index inew = 0;
      const auto oldOffset = c * oldSize;
      const auto newOffset = c * newSize;
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

VariableConcept::VariableConcept(const Dimensions &dimensions)
    : m_dimensions(dimensions){};

// Some types such as Dataset support `+` (effectively appending table rows),
// but are not arithmetic.
class AddableVariableConcept : public VariableConcept {
public:
  static constexpr const char *name = "addable";
  using VariableConcept::VariableConcept;
  virtual VariableConcept &operator+=(const VariableConcept &other) = 0;
};

class ArithmeticVariableConcept : public AddableVariableConcept {
public:
  static constexpr const char *name = "arithmetic";
  using AddableVariableConcept::AddableVariableConcept;
  virtual VariableConcept &operator-=(const VariableConcept &other) = 0;
  virtual VariableConcept &operator*=(const VariableConcept &other) = 0;
  virtual VariableConcept &operator/=(const VariableConcept &other) = 0;
};

class FloatingPointVariableConcept : public ArithmeticVariableConcept {
public:
  static constexpr const char *name = "floating-point";
  using ArithmeticVariableConcept::ArithmeticVariableConcept;
  /// Set x = value/x
  virtual VariableConcept &reciprocal_times(const double value) = 0;
  virtual void rebin(const VariableConcept &old, const Dim dim,
                     const VariableConcept &oldCoord,
                     const VariableConcept &newCoord) = 0;
};

template <class T> class ViewModel;
template <class T> class AddableVariableConceptT;
template <class T> class ArithmeticVariableConceptT;
template <class T> class FloatingPointVariableConceptT;

template <class T, typename Enable = void> struct concept {
  using type = VariableConcept;
  using typeT = VariableConceptT<T>;
};
template <class T>
struct concept<T, std::enable_if_t<std::is_same<T, Dataset>::value>> {
  using type = AddableVariableConcept;
  using typeT = AddableVariableConceptT<T>;
};
template <class T>
struct concept<T, std::enable_if_t<std::is_integral<T>::value>> {
  using type = ArithmeticVariableConcept;
  using typeT = ArithmeticVariableConceptT<T>;
};
template <class T>
struct concept<T, std::enable_if_t<std::is_floating_point<T>::value>> {
  using type = FloatingPointVariableConcept;
  using typeT = FloatingPointVariableConceptT<T>;
};

template <class T> using concept_t = typename concept<T>::type;
template <class T> using conceptT_t = typename concept<T>::typeT;

/// Partially typed implementation of VariableConcept. This is a common base
/// class for DataModel<T> and ViewModel<T>. The former holds data in a
/// contiguous array, whereas the latter is a (potentially non-contiguous) view
/// into the former. This base class implements functionality that is common to
/// both, for a specific T.
template <class T> class VariableConceptT : public concept_t<T> {
public:
  VariableConceptT(const Dimensions &dimensions) : concept_t<T>(dimensions) {}

  DType dtype() const noexcept override { return ::dtype<T>; }

  virtual gsl::span<T> getSpan() = 0;
  virtual gsl::span<T> getSpan(const Dim dim, const gsl::index begin,
                               const gsl::index end) = 0;
  virtual gsl::span<const T> getSpan() const = 0;
  virtual gsl::span<const T> getSpan(const Dim dim, const gsl::index begin,
                                     const gsl::index end) const = 0;
  virtual VariableView<T> getView(const Dimensions &dims) = 0;
  virtual VariableView<T> getView(const Dimensions &dims, const Dim dim,
                                  const gsl::index begin) = 0;
  virtual VariableView<const T> getView(const Dimensions &dims) const = 0;
  virtual VariableView<const T> getView(const Dimensions &dims, const Dim dim,
                                        const gsl::index begin) const = 0;
  virtual VariableView<const T> getReshaped(const Dimensions &dims) const = 0;
  virtual VariableView<T> getReshaped(const Dimensions &dims) = 0;

  std::unique_ptr<VariableConcept> makeView() const override {
    auto &dims = this->dimensions();
    return std::make_unique<ViewModel<decltype(getView(dims))>>(dims,
                                                                getView(dims));
  }

  std::unique_ptr<VariableConcept> makeView() override {
    if (this->isConstView())
      return const_cast<const VariableConceptT &>(*this).makeView();
    auto &dims = this->dimensions();
    return std::make_unique<ViewModel<decltype(getView(dims))>>(dims,
                                                                getView(dims));
  }

  std::unique_ptr<VariableConcept>
  makeView(const Dim dim, const gsl::index begin,
           const gsl::index end) const override {
    auto dims = this->dimensions();
    if (end == -1)
      dims.erase(dim);
    else
      dims.resize(dim, end - begin);
    return std::make_unique<ViewModel<decltype(getView(dims, dim, begin))>>(
        dims, getView(dims, dim, begin));
  }

  std::unique_ptr<VariableConcept> makeView(const Dim dim,
                                            const gsl::index begin,
                                            const gsl::index end) override {
    if (this->isConstView())
      return const_cast<const VariableConceptT &>(*this).makeView(dim, begin,
                                                                  end);
    auto dims = this->dimensions();
    if (end == -1)
      dims.erase(dim);
    else
      dims.resize(dim, end - begin);
    return std::make_unique<ViewModel<decltype(getView(dims, dim, begin))>>(
        dims, getView(dims, dim, begin));
  }

  std::unique_ptr<VariableConcept>
  reshape(const Dimensions &dims) const override {
    if (this->dimensions().volume() != dims.volume())
      throw std::runtime_error(
          "Cannot reshape to dimensions with different volume");
    return std::make_unique<ViewModel<decltype(getReshaped(dims))>>(
        dims, getReshaped(dims));
  }

  std::unique_ptr<VariableConcept> reshape(const Dimensions &dims) override {
    if (this->dimensions().volume() != dims.volume())
      throw std::runtime_error(
          "Cannot reshape to dimensions with different volume");
    return std::make_unique<ViewModel<decltype(getReshaped(dims))>>(
        dims, getReshaped(dims));
  }

  bool operator==(const VariableConcept &other) const override {
    const auto &dims = this->dimensions();
    if (dims != other.dimensions())
      return false;
    const auto &otherT = dynamic_cast<const VariableConceptT &>(other);
    if (this->isContiguous()) {
      if (other.isContiguous() && dims.isContiguousIn(other.dimensions())) {
        return equal(getSpan(), otherT.getSpan());
      } else {
        return equal(getSpan(), otherT.getView(dims));
      }
    } else {
      if (other.isContiguous() && dims.isContiguousIn(other.dimensions())) {
        return equal(getView(dims), otherT.getSpan());
      } else {
        return equal(getView(dims), otherT.getView(dims));
      }
    }
  }

  void copy(const VariableConcept &other, const Dim dim,
            const gsl::index offset, const gsl::index otherBegin,
            const gsl::index otherEnd) override {
    auto iterDims = this->dimensions();
    const gsl::index delta = otherEnd - otherBegin;
    if (iterDims.contains(dim))
      iterDims.resize(dim, delta);

    const auto &otherT = dynamic_cast<const VariableConceptT &>(other);
    auto otherView = otherT.getView(iterDims, dim, otherBegin);
    // Four cases for minimizing use of VariableView --- just copy contiguous
    // range where possible.
    if (this->isContiguous() && iterDims.isContiguousIn(this->dimensions())) {
      auto target = getSpan(dim, offset, offset + delta);
      if (other.isContiguous() && iterDims.isContiguousIn(other.dimensions())) {
        auto source = otherT.getSpan(dim, otherBegin, otherEnd);
        std::copy(source.begin(), source.end(), target.begin());
      } else {
        std::copy(otherView.begin(), otherView.end(), target.begin());
      }
    } else {
      auto view = getView(iterDims, dim, offset);
      if (other.isContiguous() && iterDims.isContiguousIn(other.dimensions())) {
        auto source = otherT.getSpan(dim, otherBegin, otherEnd);
        std::copy(source.begin(), source.end(), view.begin());
      } else {
        std::copy(otherView.begin(), otherView.end(), view.begin());
      }
    }
  }
};

template <class T> class AddableVariableConceptT : public VariableConceptT<T> {
public:
  using VariableConceptT<T>::VariableConceptT;

  template <template <class> class Op>
  VariableConcept &apply(const VariableConcept &other) {
    const auto &dims = this->dimensions();
    try {
      const auto &otherT = dynamic_cast<const VariableConceptT<T> &>(other);
      if (this->getView(dims).overlaps(otherT.getView(dims))) {
        // If there is an overlap between lhs and rhs we copy the rhs before
        // applying the operation.
        const auto &data = otherT.getView(otherT.dimensions());
        DataModel<Vector<T>> copy(other.dimensions(),
                                  Vector<T>(data.begin(), data.end()));
        return apply<Op>(copy);
      }

      if (this->isContiguous() && dims.contains(other.dimensions())) {
        if (other.isContiguous() && dims.isContiguousIn(other.dimensions())) {
          ArithmeticHelper<Op, T>::apply(this->getSpan(), otherT.getSpan());
        } else {
          ArithmeticHelper<Op, T>::apply(this->getSpan(), otherT.getView(dims));
        }
      } else if (dims.contains(other.dimensions())) {
        if (other.isContiguous() && dims.isContiguousIn(other.dimensions())) {
          ArithmeticHelper<Op, T>::apply(this->getView(dims), otherT.getSpan());
        } else {
          ArithmeticHelper<Op, T>::apply(this->getView(dims),
                                         otherT.getView(dims));
        }
      } else {
        // LHS has fewer dimensions than RHS, e.g., for computing sum. Use view.
        if (other.isContiguous() && dims.isContiguousIn(other.dimensions())) {
          ArithmeticHelper<Op, T>::apply(this->getView(other.dimensions()),
                                         otherT.getSpan());
        } else {
          ArithmeticHelper<Op, T>::apply(this->getView(other.dimensions()),
                                         otherT.getView(other.dimensions()));
        }
      }
    } catch (const std::bad_cast &) {
      throw std::runtime_error("Cannot apply arithmetic operation to "
                               "Variables: Underlying data types do not "
                               "match.");
    }
    return *this;
  }

  VariableConcept &operator+=(const VariableConcept &other) override {
    return apply<std::plus>(other);
  }
};

template <class T>
class ArithmeticVariableConceptT : public AddableVariableConceptT<T> {
public:
  using AddableVariableConceptT<T>::AddableVariableConceptT;

  VariableConcept &operator-=(const VariableConcept &other) override {
    return this->template apply<std::minus>(other);
  }

  VariableConcept &operator*=(const VariableConcept &other) override {
    return this->template apply<std::multiplies>(other);
  }

  VariableConcept &operator/=(const VariableConcept &other) override {
    return this->template apply<std::divides>(other);
  }
};

template <class T> struct ReciprocalTimes {
  T operator()(const T a, const T b) { return b / a; };
};

template <class T>
class FloatingPointVariableConceptT : public ArithmeticVariableConceptT<T> {
public:
  using ArithmeticVariableConceptT<T>::ArithmeticVariableConceptT;

  VariableConcept &reciprocal_times(const double value) override {
    Variable other(Data::Value, {}, {value});
    return this->template apply<ReciprocalTimes>(other.data());
  }

  void rebin(const VariableConcept &old, const Dim dim,
             const VariableConcept &oldCoord,
             const VariableConcept &newCoord) override {
    // Dimensions of *this and old are guaranteed to be the same.
    const auto &oldT = dynamic_cast<const FloatingPointVariableConceptT &>(old);
    const auto &oldCoordT =
        dynamic_cast<const FloatingPointVariableConceptT &>(oldCoord);
    const auto &newCoordT =
        dynamic_cast<const FloatingPointVariableConceptT &>(newCoord);
    if (this->dimensions().label(0) == dim &&
        oldCoord.dimensions().count() == 1 &&
        newCoord.dimensions().count() == 1) {
      RebinHelper<T>::rebinInner(dim, oldT, *this, oldCoordT, newCoordT);
    } else {
      auto oldCoordDims = oldCoord.dimensions();
      oldCoordDims.resize(dim, oldCoordDims[dim] - 1);
      auto newCoordDims = newCoord.dimensions();
      newCoordDims.resize(dim, newCoordDims[dim] - 1);
      auto oldCoordView = oldCoordT.getView(this->dimensions());
      auto newCoordView = newCoordT.getView(this->dimensions());
      const auto oldOffset = oldCoordDims.offset(dim);
      const auto newOffset = newCoordDims.offset(dim);

      RebinHelper<T>::rebin(dim, oldT.getSpan(), this->getSpan(), oldCoordView,
                            oldOffset, newCoordView, newOffset);
    }
  }
};

template <class T>
auto makeSpan(T &model, const Dimensions &dims, const Dim dim,
              const gsl::index begin, const gsl::index end) {
  if (!dims.contains(dim) && (begin != 0 || end != 1))
    throw std::runtime_error("VariableConcept: Slice index out of range.");
  if (!dims.contains(dim) || dims[dim] == end - begin) {
    return gsl::make_span(model.data(), model.data() + model.size());
  }
  const gsl::index beginOffset = begin * dims.offset(dim);
  const gsl::index endOffset = end * dims.offset(dim);
  return gsl::make_span(model.data() + beginOffset, model.data() + endOffset);
}

/// Implementation of VariableConcept that holds data.
template <class T> class DataModel : public conceptT_t<typename T::value_type> {
  using value_type = std::remove_const_t<typename T::value_type>;

public:
  DataModel(const Dimensions &dimensions, T model)
      : conceptT_t<typename T::value_type>(std::move(dimensions)),
        m_model(std::move(model)) {
    if (this->dimensions().volume() != static_cast<gsl::index>(m_model.size()))
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
  }

  gsl::span<value_type> getSpan() override {
    return gsl::make_span(m_model.data(), m_model.data() + size());
  }
  gsl::span<value_type> getSpan(const Dim dim, const gsl::index begin,
                                const gsl::index end) override {
    return makeSpan(m_model, this->dimensions(), dim, begin, end);
  }

  gsl::span<const value_type> getSpan() const override {
    return gsl::make_span(m_model.data(), m_model.data() + size());
  }
  gsl::span<const value_type> getSpan(const Dim dim, const gsl::index begin,
                                      const gsl::index end) const override {
    return makeSpan(m_model, this->dimensions(), dim, begin, end);
  }

  VariableView<value_type> getView(const Dimensions &dims) override {
    return makeVariableView(m_model.data(), 0, dims, this->dimensions());
  }
  VariableView<value_type> getView(const Dimensions &dims, const Dim dim,
                                   const gsl::index begin) override {
    gsl::index beginOffset = this->dimensions().contains(dim)
                                 ? begin * this->dimensions().offset(dim)
                                 : begin * this->dimensions().volume();
    return makeVariableView(m_model.data(), beginOffset, dims,
                            this->dimensions());
  }

  VariableView<const value_type>
  getView(const Dimensions &dims) const override {
    return makeVariableView(m_model.data(), 0, dims, this->dimensions());
  }
  VariableView<const value_type>
  getView(const Dimensions &dims, const Dim dim,
          const gsl::index begin) const override {
    gsl::index beginOffset = this->dimensions().contains(dim)
                                 ? begin * this->dimensions().offset(dim)
                                 : begin * this->dimensions().volume();
    return makeVariableView(m_model.data(), beginOffset, dims,
                            this->dimensions());
  }

  VariableView<const value_type>
  getReshaped(const Dimensions &dims) const override {
    return makeVariableView(m_model.data(), 0, dims, dims);
  }
  VariableView<value_type> getReshaped(const Dimensions &dims) override {
    return makeVariableView(m_model.data(), 0, dims, dims);
  }

  std::unique_ptr<VariableConcept> clone() const override {
    return std::make_unique<DataModel<T>>(this->dimensions(), m_model);
  }

  std::unique_ptr<VariableConcept>
  clone(const Dimensions &dims) const override {
    return std::make_unique<DataModel<T>>(dims, T(dims.volume()));
  }

  bool isContiguous() const override { return true; }
  bool isView() const override { return false; }
  bool isConstView() const override { return false; }

  gsl::index size() const override { return m_model.size(); }

  T m_model;
};

/// Implementation of VariableConcept that represents a view onto data.
template <class T>
class ViewModel
    : public conceptT_t<std::remove_const_t<typename T::element_type>> {
  using value_type = typename T::value_type;

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
  ViewModel(const Dimensions &dimensions, T model)
      : conceptT_t<value_type>(std::move(dimensions)),
        m_model(std::move(model)) {
    if (this->dimensions().volume() != m_model.size())
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
  }

  gsl::span<value_type> getSpan() override {
    requireMutable();
    requireContiguous();
    if constexpr (std::is_const<typename T::element_type>::value)
      return gsl::span<value_type>();
    else
      return gsl::make_span(m_model.data(), m_model.data() + size());
  }
  gsl::span<value_type> getSpan(const Dim dim, const gsl::index begin,
                                const gsl::index end) override {
    requireMutable();
    requireContiguous();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dim);
      static_cast<void>(begin);
      static_cast<void>(end);
      return gsl::span<value_type>();
    } else {
      return makeSpan(m_model, this->dimensions(), dim, begin, end);
    }
  }

  gsl::span<const value_type> getSpan() const override {
    requireContiguous();
    return gsl::make_span(m_model.data(), m_model.data() + size());
  }
  gsl::span<const value_type> getSpan(const Dim dim, const gsl::index begin,
                                      const gsl::index end) const override {
    requireContiguous();
    return makeSpan(m_model, this->dimensions(), dim, begin, end);
  }

  VariableView<value_type> getView(const Dimensions &dims) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dims);
      return VariableView<value_type>(nullptr, 0, {}, {});
    } else {
      return {m_model, dims};
    }
  }
  VariableView<value_type> getView(const Dimensions &dims, const Dim dim,
                                   const gsl::index begin) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dim);
      static_cast<void>(begin);
      return VariableView<value_type>(nullptr, 0, {}, {});
    } else {
      return {m_model, dims, dim, begin};
    }
  }

  VariableView<const value_type>
  getView(const Dimensions &dims) const override {
    return {m_model, dims};
  }
  VariableView<const value_type>
  getView(const Dimensions &dims, const Dim dim,
          const gsl::index begin) const override {
    return {m_model, dims, dim, begin};
  }

  VariableView<const value_type>
  getReshaped(const Dimensions &dims) const override {
    return {m_model, dims};
  }
  VariableView<value_type> getReshaped(const Dimensions &dims) override {
    requireMutable();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dims);
      return VariableView<value_type>(nullptr, 0, {}, {});
    } else {
      return {m_model, dims};
    }
  }

  std::unique_ptr<VariableConcept> clone() const override {
    return std::make_unique<ViewModel<T>>(this->dimensions(), m_model);
  }

  std::unique_ptr<VariableConcept> clone(const Dimensions &) const override {
    throw std::runtime_error("Cannot resize view.");
  }

  bool isContiguous() const override {
    return this->dimensions().isContiguousIn(m_model.parentDimensions());
  }
  bool isView() const override { return true; }
  bool isConstView() const override {
    return std::is_const<typename T::element_type>::value;
  }

  gsl::index size() const override { return m_model.size(); }

  T m_model;
};

Variable::Variable(const ConstVariableSlice &slice)
    : Variable(*slice.m_variable) {
  if (slice.m_view) {
    m_tag = slice.tag();
    m_name = slice.m_variable->m_name;
    setUnit(slice.unit());
    setDimensions(slice.dimensions());
    data().copy(slice.data(), Dim::Invalid, 0, 0, 1);
  }
}

template <class T>
Variable::Variable(const Tag tag, const Unit::Id unit,
                   const Dimensions &dimensions, T object)
    : m_tag(tag), m_unit{unit},
      m_object(std::make_unique<DataModel<T>>(std::move(dimensions),
                                              std::move(object))) {}

void Variable::setDimensions(const Dimensions &dimensions) {
  if (dimensions.volume() == m_object->dimensions().volume()) {
    if (dimensions != m_object->dimensions())
      data().m_dimensions = dimensions;
    return;
  }
  m_object = m_object->clone(dimensions);
}

template <class T> const Vector<T> &Variable::cast() const {
  return dynamic_cast<const DataModel<Vector<T>> &>(*m_object).m_model;
}

template <class T> Vector<T> &Variable::cast() {
  return dynamic_cast<DataModel<Vector<T>> &>(*m_object).m_model;
}

#define INSTANTIATE(...)                                                       \
  template Variable::Variable(const Tag, const Unit::Id, const Dimensions &,   \
                              Vector<__VA_ARGS__>);                            \
  template Vector<__VA_ARGS__> &Variable::cast<__VA_ARGS__>();                 \
  template const Vector<__VA_ARGS__> &Variable::cast<__VA_ARGS__>() const;

INSTANTIATE(std::string)
INSTANTIATE(double)
INSTANTIATE(float)
INSTANTIATE(char)
INSTANTIATE(int32_t)
INSTANTIATE(int64_t)
INSTANTIATE(std::pair<int64_t, int64_t>)
INSTANTIATE(ValueWithDelta<double>)
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
INSTANTIATE(gsl::index)
INSTANTIATE(std::pair<gsl::index, gsl::index>)
#endif
INSTANTIATE(boost::container::small_vector<gsl::index, 1>)
INSTANTIATE(boost::container::small_vector<double, 8>)
INSTANTIATE(std::vector<std::string>)
INSTANTIATE(std::vector<gsl::index>)
INSTANTIATE(Dataset)
INSTANTIATE(std::array<double, 3>)
INSTANTIATE(std::array<double, 4>)
INSTANTIATE(Eigen::Vector3d)

template <class T1, class T2> bool equals(const T1 &a, const T2 &b) {
  // Compare even before pointer comparison since data may be shared even if
  // names differ.
  if (a.name() != b.name())
    return false;
  if (a.unit() != b.unit())
    return false;
  // Deep comparison
  if (a.tag() != b.tag())
    return false;
  if (!(a.dimensions() == b.dimensions()))
    return false;
  return a.data() == b.data();
}

bool Variable::operator==(const Variable &other) const {
  // Compare even before pointer comparison since data may be shared even if
  // names differ.
  if (name() != other.name())
    return false;
  if (unit() != other.unit())
    return false;
  // Deep comparison
  if (tag() != other.tag())
    return false;
  if (!(dimensions() == other.dimensions()))
    return false;
  return data() == other.data();
}

bool Variable::operator==(const ConstVariableSlice &other) const {
  return equals(*this, other);
}

bool Variable::operator!=(const Variable &other) const {
  return !(*this == other);
}

bool Variable::operator!=(const ConstVariableSlice &other) const {
  return !(*this == other);
}

template <class T, class C> auto &require(C &concept) {
  try {
    return dynamic_cast<T &>(concept);
  } catch (const std::bad_cast &) {
    throw std::runtime_error(std::string("Cannot apply operation, requires ") +
                             T::name + " type.");
  }
}

template <class T1, class T2> T1 &plus_equals(T1 &variable, const T2 &other) {
  // Addition with different Variable type is supported, mismatch of underlying
  // element types is handled in DataModel::operator+=.
  // Different name is ok for addition.
  dataset::expect::equals(variable.unit(), other.unit());
  // TODO How should attributes be handled?
  if (variable.dtype() != dtype<Dataset> || variable.isAttr()) {
    dataset::expect::contains(variable.dimensions(), other.dimensions());
    // Note: This will broadcast/transpose the RHS if required. We do not
    // support changing the dimensions of the LHS though!
    require<AddableVariableConcept>(variable.data()) += other.data();
  } else {
    if (variable.dimensions() == other.dimensions()) {
      using ConstViewOrRef =
          std::conditional_t<std::is_same<T2, Variable>::value,
                             const Vector<Dataset> &,
                             const VariableView<const Dataset>>;
      ConstViewOrRef otherDatasets = other.template cast<Dataset>();
      if (otherDatasets.size() > 0 &&
          otherDatasets[0].dimensions().count() != 1)
        throw std::runtime_error(
            "Cannot add Variable: Nested Dataset dimension must be 1.");
      auto datasets = variable.template cast<Dataset>();
      const Dim dim = datasets[0].dimensions().label(0);
#pragma omp parallel for
      for (gsl::index i = 0; i < static_cast<gsl::index>(datasets.size()); ++i)
        datasets[i] = concatenate(datasets[i], otherDatasets[i], dim);
    } else {
      throw std::runtime_error(
          "Cannot add Variables: Dimensions do not match.");
    }
  }
  return variable;
}

Variable Variable::operator-() const {
  // TODO This implementation only works for variables containing doubles and
  // will throw, e.g., for ints.
  auto copy(*this);
  copy *= -1.0;
  return copy;
}

Variable &Variable::operator+=(const Variable &other) & {
  return plus_equals(*this, other);
}
Variable &Variable::operator+=(const ConstVariableSlice &other) & {
  return plus_equals(*this, other);
}
Variable &Variable::operator+=(const double value) & {
  // TODO By not setting a unit here this operator is only usable if the
  // variable is dimensionless. Should we ignore the unit for scalar operations,
  // i.e., set the same unit as *this.unit()?
  Variable other(Data::Value, {}, {value});
  return plus_equals(*this, other);
}

template <class T1, class T2> T1 &minus_equals(T1 &variable, const T2 &other) {
  dataset::expect::equals(variable.unit(), other.unit());
  dataset::expect::contains(variable.dimensions(), other.dimensions());
  if (variable.tag() == Data::Events)
    throw std::runtime_error("Subtraction of events lists not implemented.");
  require<ArithmeticVariableConcept>(variable.data()) -= other.data();
  return variable;
}

Variable &Variable::operator-=(const Variable &other) & {
  return minus_equals(*this, other);
}
Variable &Variable::operator-=(const ConstVariableSlice &other) & {
  return minus_equals(*this, other);
}
Variable &Variable::operator-=(const double value) & {
  Variable other(Data::Value, {}, {value});
  return minus_equals(*this, other);
}

template <class T1, class T2> T1 &times_equals(T1 &variable, const T2 &other) {
  dataset::expect::contains(variable.dimensions(), other.dimensions());
  if (variable.tag() == Data::Events)
    throw std::runtime_error("Multiplication of events lists not implemented.");
  // setUnit is catching bad cases of changing units (if `variable` is a slice).
  variable.setUnit(variable.unit() * other.unit());
  require<ArithmeticVariableConcept>(variable.data()) *= other.data();
  return variable;
}

Variable &Variable::operator*=(const Variable &other) & {
  return times_equals(*this, other);
}
Variable &Variable::operator*=(const ConstVariableSlice &other) & {
  return times_equals(*this, other);
}
Variable &Variable::operator*=(const double value) & {
  Variable other(Data::Value, {}, {value});
  other.setUnit(Unit::Id::Dimensionless);
  return times_equals(*this, other);
}

template <class T1, class T2> T1 &divide_equals(T1 &variable, const T2 &other) {
  dataset::expect::contains(variable.dimensions(), other.dimensions());
  if (variable.tag() == Data::Events)
    throw std::runtime_error("Division of events lists not implemented.");
  // setUnit is catching bad cases of changing units (if `variable` is a slice).
  variable.setUnit(variable.unit() / other.unit());
  require<ArithmeticVariableConcept>(variable.data()) /= other.data();
  return variable;
}

Variable &Variable::operator/=(const Variable &other) & {
  return divide_equals(*this, other);
}
Variable &Variable::operator/=(const ConstVariableSlice &other) & {
  return divide_equals(*this, other);
}
Variable &Variable::operator/=(const double value) & {
  Variable other(Data::Value, {}, {value});
  other.setUnit(Unit::Id::Dimensionless);
  return divide_equals(*this, other);
}

template <class T> VariableSlice VariableSlice::assign(const T &other) const {
  // TODO Should mismatching tags be allowed, as long as the type matches?
  if (tag() != other.tag())
    throw std::runtime_error("Cannot assign to slice: Type mismatch.");
  // Name mismatch ok, but do not assign it.
  if (unit() != other.unit())
    throw std::runtime_error("Cannot assign to slice: Unit mismatch.");
  if (dimensions() != other.dimensions())
    throw dataset::except::DimensionMismatchError(dimensions(),
                                                  other.dimensions());
  data().copy(other.data(), Dim::Invalid, 0, 0, 1);
  return *this;
}

template VariableSlice VariableSlice::assign(const Variable &) const;
template VariableSlice VariableSlice::assign(const ConstVariableSlice &) const;

VariableSlice VariableSlice::operator+=(const Variable &other) const {
  return plus_equals(*this, other);
}
VariableSlice VariableSlice::operator+=(const ConstVariableSlice &other) const {
  return plus_equals(*this, other);
}
VariableSlice VariableSlice::operator+=(const double value) const {
  Variable other(Data::Value, {}, {value});
  other.setUnit(Unit::Id::Dimensionless);
  return plus_equals(*this, other);
}

VariableSlice VariableSlice::operator-=(const Variable &other) const {
  return minus_equals(*this, other);
}
VariableSlice VariableSlice::operator-=(const ConstVariableSlice &other) const {
  return minus_equals(*this, other);
}
VariableSlice VariableSlice::operator-=(const double value) const {
  Variable other(Data::Value, {}, {value});
  other.setUnit(Unit::Id::Dimensionless);
  return minus_equals(*this, other);
}

VariableSlice VariableSlice::operator*=(const Variable &other) const {
  return times_equals(*this, other);
}
VariableSlice VariableSlice::operator*=(const ConstVariableSlice &other) const {
  return times_equals(*this, other);
}
VariableSlice VariableSlice::operator*=(const double value) const {
  Variable other(Data::Value, {}, {value});
  other.setUnit(Unit::Id::Dimensionless);
  return times_equals(*this, other);
}

VariableSlice VariableSlice::operator/=(const Variable &other) const {
  return divide_equals(*this, other);
}
VariableSlice VariableSlice::operator/=(const ConstVariableSlice &other) const {
  return divide_equals(*this, other);
}
VariableSlice VariableSlice::operator/=(const double value) const {
  Variable other(Data::Value, {}, {value});
  other.setUnit(Unit::Id::Dimensionless);
  return divide_equals(*this, other);
}

bool ConstVariableSlice::operator==(const Variable &other) const {
  // Always use deep comparison (pointer comparison does not make sense since we
  // may be looking at a different section).
  return equals(*this, other);
}
bool ConstVariableSlice::operator==(const ConstVariableSlice &other) const {
  return equals(*this, other);
}

bool ConstVariableSlice::operator!=(const Variable &other) const {
  return !(*this == other);
}
bool ConstVariableSlice::operator!=(const ConstVariableSlice &other) const {
  return !(*this == other);
}

Variable ConstVariableSlice::operator-() const {
  Variable copy(*this);
  return -copy;
}

void VariableSlice::setUnit(const Unit &unit) const {
  // TODO Should we forbid setting the unit altogether? I think it is useful in
  // particular since views onto subsets of dataset do not imply slicing of
  // variables but return slice views.
  if ((this->unit() != unit) &&
      (dimensions() != m_mutableVariable->dimensions()))
    throw std::runtime_error("Partial view on data of variable cannot be used "
                             "to change the unit.\n");
  m_mutableVariable->setUnit(unit);
}

template <class T>
const VariableView<const T> ConstVariableSlice::cast() const {
  if (!m_view)
    return dynamic_cast<const DataModel<Vector<T>> &>(data()).getView(
        dimensions());
  if (m_view->isConstView())
    return dynamic_cast<const ViewModel<VariableView<const T>> &>(data())
        .m_model;
  // Make a const view from the mutable one.
  return {dynamic_cast<const ViewModel<VariableView<T>> &>(data()).m_model,
          dimensions()};
}

template <class T> VariableView<T> VariableSlice::cast() const {
  if (m_view)
    return dynamic_cast<const ViewModel<VariableView<T>> &>(data()).m_model;
  return dynamic_cast<DataModel<Vector<T>> &>(data()).getView(dimensions());
}

#define INSTANTIATE_SLICEVIEW(...)                                             \
  template const VariableView<const __VA_ARGS__>                               \
  ConstVariableSlice::cast<__VA_ARGS__>() const;                               \
  template VariableView<__VA_ARGS__> VariableSlice::cast() const;

INSTANTIATE_SLICEVIEW(double);
INSTANTIATE_SLICEVIEW(float);
INSTANTIATE_SLICEVIEW(int64_t);
INSTANTIATE_SLICEVIEW(int32_t);
INSTANTIATE_SLICEVIEW(char);
INSTANTIATE_SLICEVIEW(std::string);

ConstVariableSlice Variable::operator()(const Dim dim, const gsl::index begin,
                                        const gsl::index end) const & {
  return {*this, dim, begin, end};
}

VariableSlice Variable::operator()(const Dim dim, const gsl::index begin,
                                   const gsl::index end) & {
  return {*this, dim, begin, end};
}

ConstVariableSlice Variable::reshape(const Dimensions &dims) const & {
  return {*this, dims};
}

VariableSlice Variable::reshape(const Dimensions &dims) & {
  return {*this, dims};
}

Variable Variable::reshape(const Dimensions &dims) && {
  Variable reshaped(std::move(*this));
  reshaped.setDimensions(dims);
  return reshaped;
}

Variable ConstVariableSlice::reshape(const Dimensions &dims) const {
  // In general a variable slice is not contiguous. Therefore we cannot reshape
  // without making a copy (except for special cases).
  Variable reshaped(*this);
  reshaped.setDimensions(dims);
  return reshaped;
}

// Note: The std::move here is necessary because RVO does not work for variables
// that are function parameters.
Variable operator+(Variable a, const Variable &b) { return std::move(a += b); }
Variable operator-(Variable a, const Variable &b) { return std::move(a -= b); }
Variable operator*(Variable a, const Variable &b) { return std::move(a *= b); }
Variable operator/(Variable a, const Variable &b) { return std::move(a /= b); }
Variable operator+(Variable a, const ConstVariableSlice &b) {
  return std::move(a += b);
}
Variable operator-(Variable a, const ConstVariableSlice &b) {
  return std::move(a -= b);
}
Variable operator*(Variable a, const ConstVariableSlice &b) {
  return std::move(a *= b);
}
Variable operator/(Variable a, const ConstVariableSlice &b) {
  return std::move(a /= b);
}
Variable operator+(Variable a, const double b) { return std::move(a += b); }
Variable operator-(Variable a, const double b) { return std::move(a -= b); }
Variable operator*(Variable a, const double b) { return std::move(a *= b); }
Variable operator/(Variable a, const double b) { return std::move(a /= b); }
Variable operator+(const double a, Variable b) { return std::move(b += a); }
Variable operator-(const double a, Variable b) { return -(b -= a); }
Variable operator*(const double a, Variable b) { return std::move(b *= a); }
Variable operator/(const double a, Variable b) {
  b.setUnit(Unit::Id::Dimensionless / b.unit());
  require<FloatingPointVariableConcept>(b.data()).reciprocal_times(a);
  return std::move(b);
}

// Example of a "derived" operation: Implementation does not require adding a
// virtual function to VariableConcept.
std::vector<Variable> split(const Variable &var, const Dim dim,
                            const std::vector<gsl::index> &indices) {
  if (indices.empty())
    return {var};
  std::vector<Variable> vars;
  vars.emplace_back(var(dim, 0, indices.front()));
  for (gsl::index i = 0; i < static_cast<gsl::index>(indices.size()) - 1; ++i)
    vars.emplace_back(var(dim, indices[i], indices[i + 1]));
  vars.emplace_back(var(dim, indices.back(), var.dimensions()[dim]));
  return vars;
}

Variable concatenate(const Variable &a1, const Variable &a2, const Dim dim) {
  if (a1.tag() != a2.tag())
    throw std::runtime_error(
        "Cannot concatenate Variables: Data types do not match.");
  if (a1.unit() != a2.unit())
    throw std::runtime_error(
        "Cannot concatenate Variables: Units do not match.");
  if (a1.name() != a2.name())
    throw std::runtime_error(
        "Cannot concatenate Variables: Names do not match.");
  const auto &dims1 = a1.dimensions();
  const auto &dims2 = a2.dimensions();
  // TODO Many things in this function should be refactored and moved in class
  // Dimensions.
  // TODO Special handling for edge variables.
  for (const auto &dim1 : dims1.labels()) {
    if (dim1 != dim) {
      if (!dims2.contains(dim1))
        throw std::runtime_error(
            "Cannot concatenate Variables: Dimensions do not match.");
      if (dims2[dim1] != dims1[dim1])
        throw std::runtime_error(
            "Cannot concatenate Variables: Dimension extents do not match.");
    }
  }
  auto size1 = dims1.count();
  auto size2 = dims2.count();
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
  gsl::index extent1 = 1;
  gsl::index extent2 = 1;
  if (dims1.contains(dim))
    extent1 += dims1[dim] - 1;
  if (dims2.contains(dim))
    extent2 += dims2[dim] - 1;
  if (dims.contains(dim))
    dims.resize(dim, extent1 + extent2);
  else
    dims.add(dim, extent1 + extent2);
  out.setDimensions(dims);

  out.data().copy(a1.data(), dim, 0, 0, extent1);
  out.data().copy(a2.data(), dim, extent1, 0, extent2);

  return out;
}

Variable rebin(const Variable &var, const Variable &oldCoord,
               const Variable &newCoord) {
  auto rebinned(var);
  auto dims = rebinned.dimensions();
  const Dim dim = coordDimension[newCoord.tag().value()];
  dims.resize(dim, newCoord.dimensions()[dim] - 1);
  rebinned.setDimensions(dims);
  // TODO take into account unit if values have been divided by bin width.
  require<FloatingPointVariableConcept>(rebinned.data())
      .rebin(var.data(), dim, oldCoord.data(), newCoord.data());
  return rebinned;
}

Variable permute(const Variable &var, const Dim dim,
                 const std::vector<gsl::index> &indices) {
  auto permuted(var);
  for (size_t i = 0; i < indices.size(); ++i)
    permuted.data().copy(var.data(), dim, i, indices[i], indices[i] + 1);
  return permuted;
}

Variable filter(const Variable &var, const Variable &filter) {
  if (filter.dimensions().ndim() != 1)
    throw std::runtime_error(
        "Cannot filter variable: The filter must by 1-dimensional.");
  const auto dim = filter.dimensions().labels()[0];
  auto mask = filter.get(Coord::Mask);

  const gsl::index removed = std::count(mask.begin(), mask.end(), 0);
  if (removed == 0)
    return var;

  auto out(var);
  auto dims = out.dimensions();
  dims.resize(dim, dims[dim] - removed);
  out.setDimensions(dims);

  gsl::index iOut = 0;
  // Note: Could copy larger chunks of applicable for better(?) performance.
  // Note: This implementation is inefficient, since we need to cast to concrete
  // type for *every* slice. Should be combined into a single virtual call.
  for (gsl::index iIn = 0; iIn < mask.size(); ++iIn)
    if (mask[iIn])
      out.data().copy(var.data(), dim, iOut++, iIn, iIn + 1);
  return out;
}

Variable sum(const Variable &var, const Dim dim) {
  auto summed(var);
  auto dims = summed.dimensions();
  dims.erase(dim);
  // setDimensions zeros the data
  summed.setDimensions(dims);
  require<ArithmeticVariableConcept>(summed.data()) += var.data();
  return summed;
}

Variable mean(const Variable &var, const Dim dim) {
  auto summed = sum(var, dim);
  double scale = 1.0 / static_cast<double>(var.dimensions()[dim]);
  return summed * Variable(Data::Value, {}, {scale});
}

template <>
VariableView<const double> getView<double>(const Variable &var,
                                           const Dimensions &dims) {
  return dynamic_cast<const VariableConceptT<double> &>(var.data())
      .getView(dims);
}
