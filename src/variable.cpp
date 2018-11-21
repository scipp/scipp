/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "variable.h"
#include "dataset.h"
#include "variable_view.h"

template <class T> struct CloneHelper {
  static T getModel(const Dimensions &dims) { return T(dims.volume()); }
};

template <class T> struct CloneHelper<VariableView<T>> {
  static VariableView<T> getModel(const Dimensions &dims) {
    throw std::runtime_error("Cannot resize view.");
  }
};

template <template <class> class Op, class T> struct ArithmeticHelper {
  template <class InputView, class OutputView>
  static void apply(const OutputView &a, const InputView &b) {
    std::transform(a.begin(), a.end(), b.begin(), a.begin(), Op<T>());
  }
  // These overloads exist only to make the compiler happy, if they are ever
  // called it probably indicates that something is wrong in the call chain.
  template <class Other>
  static void apply(const gsl::span<const T> &a, const Other &b) {
    throw std::runtime_error("Cannot modify data via const view.");
  }
  template <class Other>
  static void apply(const VariableView<const T> &a, const Other &b) {
    throw std::runtime_error("Cannot modify data via const view.");
  }
  template <class Other> static void apply(const Vector<T> &a, const Other &b) {
    throw std::runtime_error("Passed vector to apply, this should not happen.");
  }
};

template <class T> struct CopyHelper {
  template <class T1, class T2> static void copy(T1 &view1, T2 &view2) {
    std::copy(view1.begin(), view1.end(), view2.begin());
  }
};

template <class T> struct CopyHelper<const T> {
  template <class T1, class T2> static void copy(T1 &view1, T2 &view2) {
    throw std::runtime_error("Cannot modify data via const view.");
  }
};

template <class T> class VariableModel;
template <class T> struct RebinHelper {
  static void
  rebin(const Dim dim, const T &oldModel, T &newModel,
        const VariableView<const typename T::value_type> &oldCoordView,
        const gsl::index oldOffset,
        const VariableView<const typename T::value_type> &newCoordView,
        const gsl::index newOffset) {

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
  static void rebinInner(const Dim dim, const VariableModel<T> &oldModel,
                         VariableModel<T> &newModel,
                         const VariableModel<T> &oldCoord,
                         const VariableModel<T> &newCoord) {
    const auto &oldData = oldModel.m_model;
    auto &newData = newModel.m_model;
    const auto oldSize = oldModel.dimensions().size(dim);
    const auto newSize = newModel.dimensions().size(dim);
    const auto count = oldModel.dimensions().volume() / oldSize;
    const auto *xold = &*oldCoord.m_model.begin();
    const auto *xnew = &*newCoord.m_model.begin();
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

#define DISABLE_ARITHMETICS_T(...)                                             \
  template <template <class> class Op, class T>                                \
  struct ArithmeticHelper<Op, __VA_ARGS__> {                                   \
    template <class... Args> static void apply(Args &&...) {                   \
      throw std::runtime_error(                                                \
          "Not an arithmetic type. Cannot apply operand.");                    \
    }                                                                          \
  };

DISABLE_ARITHMETICS_T(std::shared_ptr<T>)
DISABLE_ARITHMETICS_T(std::array<T, 4>)
DISABLE_ARITHMETICS_T(std::array<T, 3>)
DISABLE_ARITHMETICS_T(boost::container::small_vector<T, 1>)
DISABLE_ARITHMETICS_T(std::vector<T>)
DISABLE_ARITHMETICS_T(std::pair<T, T>)
DISABLE_ARITHMETICS_T(ValueWithDelta<T>)

template <template <class> class Op> struct ArithmeticHelper<Op, std::string> {
  template <class... Args> static void apply(Args &&...) {
    throw std::runtime_error("Cannot add strings. Use append() instead.");
  }
};

#define DISABLE_REBIN_T(...)                                                   \
  template <class T> struct RebinHelper<Vector<__VA_ARGS__>> {                 \
    template <class... Args> static void rebin(Args &&...) {                   \
      throw std::runtime_error("Not and arithmetic type. Cannot rebin.");      \
    }                                                                          \
    template <class... Args> static void rebinInner(Args &&...) {              \
      throw std::runtime_error("Not and arithmetic type. Cannot rebin.");      \
    }                                                                          \
  };

#define DISABLE_REBIN(...)                                                     \
  template <> struct RebinHelper<Vector<__VA_ARGS__>> {                        \
    template <class... Args> static void rebin(Args &&...) {                   \
      throw std::runtime_error("Not and arithmetic type. Cannot rebin.");      \
    }                                                                          \
    template <class... Args> static void rebinInner(Args &&...) {              \
      throw std::runtime_error("Not and arithmetic type. Cannot rebin.");      \
    }                                                                          \
  };

#define DISABLE_REBIN_VIEW()                                                   \
  template <class T> struct RebinHelper<VariableView<T>> {                     \
    template <class... Args> static void rebin(Args &&...) {                   \
      throw std::runtime_error("Cannot rebin VariableView.");                  \
    }                                                                          \
    template <class... Args> static void rebinInner(Args &&...) {              \
      throw std::runtime_error("Cannot rebin VariableView.");                  \
    }                                                                          \
  };

DISABLE_REBIN_T(std::shared_ptr<T>)
DISABLE_REBIN_T(std::array<T, 4>)
DISABLE_REBIN_T(std::array<T, 3>)
DISABLE_REBIN_T(std::vector<T>)
DISABLE_REBIN_T(boost::container::small_vector<T, 1>)
DISABLE_REBIN_T(std::pair<T, T>)
DISABLE_REBIN_T(ValueWithDelta<T>)
DISABLE_REBIN(Dataset)
DISABLE_REBIN(std::string)
DISABLE_REBIN_VIEW();

VariableConcept::VariableConcept(const Dimensions &dimensions)
    : m_dimensions(dimensions){};

template <class T> struct ViewHelper {
  static bool isView() { return false; }
  static bool isConstView() { return false; }
  static const Dimensions &parentDimensions(const T &model) {
    throw std::runtime_error("Not a view. Parent dimensions not defined.");
  }
};
template <class T> struct ViewHelper<VariableView<T>> {
  static bool isView() { return true; }
  static bool isConstView() { return false; }
  static const Dimensions &parentDimensions(const VariableView<T> &view) {
    return view.parentDimensions();
  }
};
template <class T> struct ViewHelper<VariableView<const T>> {
  static bool isView() { return true; }
  static bool isConstView() { return true; }
  static const Dimensions &parentDimensions(const VariableView<const T> &view) {
    return view.parentDimensions();
  }
};

template <class T> struct CastHelper {
  template <class Concept> static auto *getData(Concept &concept) {
    if (!concept.isView())
      return dynamic_cast<std::conditional_t<std::is_const<Concept>::value,
                                             const VariableModel<T> &,
                                             VariableModel<T> &>>(concept)
          .m_model.data();
    else
      return CastHelper<VariableView<std::conditional_t<
          std::is_const<Concept>::value, const typename T::value_type,
          typename T::value_type>>>::getData(concept);
  }

  template <class Concept> static auto getSpan(Concept &concept) {
    auto *data = CastHelper<T>::getData(concept);
    return gsl::make_span(data, data + concept.size());
  }

  template <class Concept>
  static auto getSpan(Concept &concept, const Dim dim, const gsl::index begin,
                      const gsl::index end) {
    auto *data = CastHelper<T>::getData(concept);
    if (!concept.dimensions().contains(dim) && (begin != 0 || end != 1))
      throw std::runtime_error("VariableConcept: Slice index out of range.");
    if (!concept.dimensions().contains(dim) ||
        concept.dimensions().size(dim) == end - begin) {
      return gsl::make_span(data, data + concept.size());
    }
    const auto &dims = concept.dimensions();
    gsl::index beginOffset = begin * dims.offset(dim);
    gsl::index endOffset = end * dims.offset(dim);
    return gsl::make_span(data + beginOffset, data + endOffset);
  }

  template <class Concept>
  static VariableView<
      std::conditional_t<std::is_const<Concept>::value,
                         const typename T::value_type, typename T::value_type>>
  getView(Concept &concept, const Dimensions &dims) {
    if (!concept.isView()) {
      auto *data = CastHelper<T>::getData(concept);
      return makeVariableView(data, dims, concept.dimensions());
    } else {
      return CastHelper<VariableView<std::conditional_t<
          std::is_const<Concept>::value, const typename T::value_type,
          typename T::value_type>>>::getView(concept, dims);
    }
  }

  template <class Concept>
  static auto getView(Concept &concept, const Dimensions &dims, const Dim dim,
                      const gsl::index begin) {
    auto *data = CastHelper<T>::getData(concept);
    gsl::index beginOffset = concept.dimensions().contains(dim)
                                 ? begin * concept.dimensions().offset(dim)
                                 : begin * concept.dimensions().volume();
    return makeVariableView(data + beginOffset, dims, concept.dimensions());
  }
};

template <class T> struct CastHelper<VariableView<T>> {
  template <class Concept> static auto &getModel(Concept &concept) {
    return dynamic_cast<std::conditional_t<
        std::is_const<Concept>::value, const VariableModel<VariableView<T>> &,
        VariableModel<VariableView<T>> &>>(concept)
        .m_model;
  }

  template <class Concept> static auto *getData(Concept &concept) {
    return CastHelper<VariableView<T>>::getModel(concept).data();
  }

  template <class Concept> static auto getSpan(Concept &concept) {
    if (!concept.isView())
      return CastHelper<Vector<T>>::getSpan(concept);
    auto *data = CastHelper<VariableView<T>>::getData(concept);
    return gsl::make_span(data, data + concept.size());
  }

  template <class Concept>
  static auto getSpan(Concept &concept, const Dim dim, const gsl::index begin,
                      const gsl::index end) {
    throw std::runtime_error("Creating sub-span of view is not implemented.");
    auto *data = CastHelper<VariableView<T>>::getData(concept);
    return gsl::make_span(data, data);
  }

  template <class Concept>
  static VariableView<
      std::conditional_t<std::is_const<Concept>::value, const T, T>>
  getView(Concept &concept, const Dimensions &dims) {
    if (!concept.isView())
      return CastHelper<Vector<T>>::getView(concept, dims);
    if (concept.isConstView())
      return {CastHelper<VariableView<T>>::getModel(concept), dims};
    else
      return {
          CastHelper<VariableView<std::remove_const_t<T>>>::getModel(concept),
          dims};
  }

  template <class Concept>
  static VariableView<T> getView(Concept &concept, const Dimensions &dims,
                                 const Dim dim, const gsl::index begin) {
    if (concept.isConstView())
      return {CastHelper<VariableView<T>>::getModel(concept), dims, dim, begin};
    else
      return {
          CastHelper<VariableView<std::remove_const_t<T>>>::getModel(concept),
          dims, dim, begin};
  }
};

template <class T> class VariableModel final : public VariableConcept {
public:
  VariableModel(const Dimensions &dimensions, T model)
      : VariableConcept(std::move(dimensions)), m_model(std::move(model)) {
    if (this->dimensions().volume() != m_model.size())
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
  }

  std::shared_ptr<VariableConcept> clone() const override {
    return std::make_shared<VariableModel<T>>(dimensions(), m_model);
  }

  std::shared_ptr<VariableConcept>
  clone(const Dimensions &dims) const override {
    return std::make_shared<VariableModel<T>>(dims,
                                              CloneHelper<T>::getModel(dims));
  }

  std::unique_ptr<VariableConcept> makeView() const override {
    auto &dims = dimensions();
    return std::make_unique<
        VariableModel<decltype(CastHelper<T>::getView(*this, dims))>>(
        dims, CastHelper<T>::getView(*this, dims));
  }

  std::unique_ptr<VariableConcept> makeView() override {
    auto &dims = dimensions();
    return std::make_unique<
        VariableModel<decltype(CastHelper<T>::getView(*this, dims))>>(
        dims, CastHelper<T>::getView(*this, dims));
  }

  std::unique_ptr<VariableConcept>
  makeView(const Dim dim, const gsl::index begin,
           const gsl::index end) const override {
    auto dims = dimensions();
    if (end == -1)
      dims.erase(dim);
    else
      dims.resize(dim, end - begin);
    return std::make_unique<VariableModel<decltype(
        CastHelper<T>::getView(*this, dims, dim, begin))>>(
        dims, CastHelper<T>::getView(*this, dims, dim, begin));
  }

  std::unique_ptr<VariableConcept> makeView(const Dim dim,
                                            const gsl::index begin,
                                            const gsl::index end) override {
    auto dims = dimensions();
    if (end == -1)
      dims.erase(dim);
    else
      dims.resize(dim, end - begin);
    return std::make_unique<VariableModel<decltype(
        CastHelper<T>::getView(*this, dims, dim, begin))>>(
        dims, CastHelper<T>::getView(*this, dims, dim, begin));
  }

  bool isContiguous() const override {
    if (!isView())
      return true;
    return dimensions().isContiguousIn(
        ViewHelper<T>::parentDimensions(m_model));
  }
  bool isView() const override { return ViewHelper<T>::isView(); }
  bool isConstView() const override { return ViewHelper<T>::isConstView(); }

  bool operator==(const VariableConcept &other) const override {
    return m_model == dynamic_cast<const VariableModel<T> &>(other).m_model;
  }

  template <template <class> class Op>
  VariableConcept &apply(const VariableConcept &other) {
    try {
      if (isContiguous()) {
        if (other.isContiguous() &&
            dimensions().isContiguousIn(other.dimensions())) {
          ArithmeticHelper<Op, std::remove_const_t<typename T::value_type>>::
              apply(CastHelper<T>::getSpan(*this),
                    CastHelper<T>::getSpan(other));
        } else {
          ArithmeticHelper<Op, std::remove_const_t<typename T::value_type>>::
              apply(CastHelper<T>::getSpan(*this),
                    CastHelper<T>::getView(other, dimensions()));
        }
      } else {
        if (other.isContiguous() &&
            dimensions().isContiguousIn(other.dimensions())) {
          ArithmeticHelper<Op, std::remove_const_t<typename T::value_type>>::
              apply(m_model, CastHelper<T>::getSpan(other));
        } else {
          ArithmeticHelper<Op, std::remove_const_t<typename T::value_type>>::
              apply(m_model, CastHelper<T>::getView(other, dimensions()));
        }
      }
    } catch (const std::bad_cast &) {
      throw std::runtime_error("Cannot apply arithmetic operation to "
                               "Variables: Underlying data types do not "
                               "match.");
    }
    return *this;
  }

  void rebin(const VariableConcept &old, const Dim dim,
             const VariableConcept &oldCoord,
             const VariableConcept &newCoord) override {
    // Dimensions of *this and old are guaranteed to be the same.
    if (dimensions().label(0) == dim && oldCoord.dimensions().count() == 1 &&
        newCoord.dimensions().count() == 1) {
      RebinHelper<T>::rebinInner(
          dim, dynamic_cast<const VariableModel<T> &>(old), *this,
          dynamic_cast<const VariableModel<T> &>(oldCoord),
          dynamic_cast<const VariableModel<T> &>(newCoord));
    } else {
      const auto &oldModel =
          dynamic_cast<const VariableModel<T> &>(old).m_model;
      auto oldCoordDims = oldCoord.dimensions();
      oldCoordDims.resize(dim, oldCoordDims.size(dim) - 1);
      auto newCoordDims = newCoord.dimensions();
      newCoordDims.resize(dim, newCoordDims.size(dim) - 1);
      auto oldCoordView = CastHelper<T>::getView(oldCoord, dimensions());
      auto newCoordView = CastHelper<T>::getView(newCoord, dimensions());
      const auto oldOffset = oldCoordDims.offset(dim);
      const auto newOffset = newCoordDims.offset(dim);

      RebinHelper<T>::rebin(dim, oldModel, m_model, oldCoordView, oldOffset,
                            newCoordView, newOffset);
    }
  }

  VariableConcept &operator+=(const VariableConcept &other) override {
    return apply<std::plus>(other);
  }

  VariableConcept &operator-=(const VariableConcept &other) override {
    return apply<std::minus>(other);
  }

  VariableConcept &operator*=(const VariableConcept &other) override {
    return apply<std::multiplies>(other);
  }

  gsl::index size() const override { return m_model.size(); }

  void copy(const VariableConcept &other, const Dim dim,
            const gsl::index offset, const gsl::index otherBegin,
            const gsl::index otherEnd) override {
    auto iterDims = dimensions();
    const gsl::index delta = otherEnd - otherBegin;
    if (iterDims.contains(dim))
      iterDims.resize(dim, delta);

    auto source = CastHelper<T>::getSpan(other, dim, otherBegin, otherEnd);
    auto otherView = CastHelper<T>::getView(other, iterDims, dim, otherBegin);
    // Four cases for minimizing use of VariableView --- just copy contiguous
    // range where possible.
    if (isContiguous() && iterDims.isContiguousIn(dimensions())) {
      auto target = CastHelper<T>::getSpan(*this, dim, offset, offset + delta);
      if (other.isContiguous() && iterDims.isContiguousIn(other.dimensions())) {
        CopyHelper<typename T::value_type>::copy(source, target);
      } else {
        CopyHelper<typename T::value_type>::copy(otherView, target);
      }
    } else {
      auto view = CastHelper<T>::getView(*this, iterDims, dim, offset);
      if (other.isContiguous() && iterDims.isContiguousIn(other.dimensions())) {
        CopyHelper<typename T::value_type>::copy(source, view);
      } else {
        CopyHelper<typename T::value_type>::copy(otherView, view);
      }
    }
  }

  T m_model;
};

template <class T>
Variable::Variable(uint32_t id, const Unit::Id unit,
                   const Dimensions &dimensions, T object)
    : m_type(id), m_unit{unit},
      m_object(std::make_unique<VariableModel<T>>(std::move(dimensions),
                                                  std::move(object))) {}

void Variable::setDimensions(const Dimensions &dimensions) {
  if (dimensions == m_object->dimensions())
    return;
  m_object = m_object->clone(dimensions);
}

template <class T> const T &Variable::cast() const {
  return dynamic_cast<const VariableModel<T> &>(*m_object).m_model;
}

template <class T> T &Variable::cast() {
  return dynamic_cast<VariableModel<T> &>(m_object.access()).m_model;
}

#define INSTANTIATE(...)                                                       \
  template Variable::Variable(uint32_t, const Unit::Id, const Dimensions &,    \
                              Vector<__VA_ARGS__>);                            \
  template Vector<__VA_ARGS__> &Variable::cast<Vector<__VA_ARGS__>>();         \
  template const Vector<__VA_ARGS__> &Variable::cast<Vector<__VA_ARGS__>>()    \
      const;

INSTANTIATE(std::string)
INSTANTIATE(double)
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
INSTANTIATE(std::vector<std::string>)
INSTANTIATE(std::vector<gsl::index>)
INSTANTIATE(Dataset)
INSTANTIATE(std::array<double, 3>)
INSTANTIATE(std::array<double, 4>)
INSTANTIATE(std::shared_ptr<std::array<double, 100>>)

bool Variable::operator==(const Variable &other) const {
  // Compare even before pointer comparison since data may be shared even if
  // names differ.
  if (m_name != other.m_name) {
    if (m_name == nullptr || other.m_name == nullptr)
      return false;
    if (*m_name != *other.m_name)
      return false;
  }
  if (m_unit != other.m_unit)
    return false;
  // Trivial case: Pointers are equal
  if (m_object == other.m_object)
    return true;
  // Deep comparison
  if (m_type != other.m_type)
    return false;
  if (!(dimensions() == other.dimensions()))
    return false;
  return *m_object == *other.m_object;
}

bool Variable::operator!=(const Variable &other) const {
  return !(*this == other);
}

Variable &Variable::operator+=(const Variable &other) {
  // Addition with different Variable type is supported, mismatch of underlying
  // element types is handled in VariableModel::operator+=.
  // Different name is ok for addition.
  if (m_unit != other.m_unit)
    throw std::runtime_error("Cannot add Variables: Units do not match.");
  if (!valueTypeIs<Data::Events>() && !valueTypeIs<Data::Table>()) {
    if (dimensions().contains(other.dimensions())) {
      // Note: This will broadcast/transpose the RHS if required. We do not
      // support changing the dimensions of the LHS though!
      m_object.access() += *other.m_object;
    } else {
      throw std::runtime_error(
          "Cannot add Variables: Dimensions do not match.");
    }
  } else {
    if (dimensions() == other.dimensions()) {
      const auto otherDatasets = gsl::make_span(other.cast<Vector<Dataset>>());
      if (otherDatasets.size() > 0 &&
          otherDatasets[0].dimensions().count() != 1)
        throw std::runtime_error(
            "Cannot add Variable: Nested Dataset dimension must be 1.");
      auto datasets = gsl::make_span(cast<Vector<Dataset>>());
      const Dim dim = datasets[0].dimensions().label(0);
#pragma omp parallel for
      for (gsl::index i = 0; i < datasets.size(); ++i)
        datasets[i] = concatenate(datasets[i], otherDatasets[i], dim);
    } else {
      throw std::runtime_error(
          "Cannot add Variables: Dimensions do not match.");
    }
  }

  return *this;
}

template <class T> Variable &Variable::operator-=(const T &other) {
  if (unit() != other.unit())
    throw std::runtime_error("Cannot subtract Variables: Units do not match.");
  if (dimensions().contains(other.dimensions())) {
    if (valueTypeIs<Data::Events>())
      throw std::runtime_error("Subtraction of events lists not implemented.");
    m_object.access() -= other.data();
  } else {
    throw std::runtime_error(
        "Cannot subtract Variables: Dimensions do not match.");
  }

  return *this;
}

template Variable &Variable::operator-=(const Variable &);
template Variable &Variable::operator-=(const VariableSlice<const Variable> &);
template Variable &Variable::operator-=(const VariableSlice<Variable> &);

template <class T>
VariableSliceMutableMixin<VariableSlice<Variable>> &
VariableSliceMutableMixin<VariableSlice<Variable>>::operator-=(const T &other) {
  if (base().unit() != other.unit())
    throw std::runtime_error("Cannot subtract Variables: Units do not match.");
  if (base().dimensions().contains(other.dimensions())) {
    if (base().valueTypeIs<Data::Events>())
      throw std::runtime_error("Subtraction of events lists not implemented.");
    base().data() -= other.data();
  } else {
    throw std::runtime_error(
        "Cannot subtract Variables: Dimensions do not match.");
  }

  return *this;
}

const VariableSlice<Variable> &
VariableSliceMutableMixin<VariableSlice<Variable>>::base() const {
  return static_cast<const VariableSlice<Variable> &>(*this);
}

VariableSlice<Variable> &
VariableSliceMutableMixin<VariableSlice<Variable>>::base() {
  return static_cast<VariableSlice<Variable> &>(*this);
}

template VariableSliceMutableMixin<VariableSlice<Variable>> &
VariableSliceMutableMixin<VariableSlice<Variable>>::
operator-=(const Variable &);
template VariableSliceMutableMixin<VariableSlice<Variable>> &
VariableSliceMutableMixin<VariableSlice<Variable>>::
operator-=(const VariableSlice<const Variable> &);
template VariableSliceMutableMixin<VariableSlice<Variable>> &
VariableSliceMutableMixin<VariableSlice<Variable>>::
operator-=(const VariableSlice<Variable> &);

Variable &Variable::operator*=(const Variable &other) {
  if (!dimensions().contains(other.dimensions()))
    throw std::runtime_error(
        "Cannot multiply Variables: Dimensions do not match.");
  if (valueTypeIs<Data::Events>())
    throw std::runtime_error("Multiplication of events lists not implemented.");
  m_unit = m_unit * other.m_unit;
  m_object.access() *= *other.m_object;
  return *this;
}

void Variable::setSlice(const Variable &slice, const Dimension dim,
                        const gsl::index index) {
  if (m_unit != slice.m_unit)
    throw std::runtime_error("Cannot set slice: Units do not match.");
  if (m_object == slice.m_object)
    return;
  if (!dimensions().contains(slice.dimensions()))
    throw std::runtime_error("Cannot set slice: Dimensions do not match.");
  data().copy(slice.data(), dim, index, 0, 1);
}

VariableSlice<const Variable> Variable::
operator()(const Dim dim, const gsl::index begin, const gsl::index end) const {
  return {*this, dim, begin, end};
}

VariableSlice<Variable> Variable::
operator()(const Dim dim, const gsl::index begin, const gsl::index end) {
  return {*this, dim, begin, end};
}

Variable operator+(Variable a, const Variable &b) { return a += b; }
Variable operator-(Variable a, const Variable &b) { return a -= b; }
Variable operator*(Variable a, const Variable &b) { return a *= b; }

Variable slice(const Variable &var, const Dimension dim,
               const gsl::index index) {
  auto out(var);
  auto dims = out.dimensions();
  dims.erase(dim);
  out.setDimensions(dims);
  out.data().copy(var.data(), dim, 0, index, index + 1);
  return out;
}

Variable slice(const Variable &var, const Dimension dim, const gsl::index begin,
               const gsl::index end) {
  auto out(var);
  auto dims = out.dimensions();
  dims.resize(dim, end - begin);
  if (dims == out.dimensions())
    return out;
  out.setDimensions(dims);
  out.data().copy(var.data(), dim, 0, begin, end);
  return out;
}

// Example of a "derived" operation: Implementation does not require adding a
// virtual function to VariableConcept.
std::vector<Variable> split(const Variable &var, const Dim dim,
                            const std::vector<gsl::index> &indices) {
  if (indices.empty())
    return {var};
  std::vector<Variable> vars;
  vars.emplace_back(slice(var, dim, 0, indices.front()));
  for (gsl::index i = 0; i < indices.size() - 1; ++i)
    vars.emplace_back(slice(var, dim, indices[i], indices[i + 1]));
  vars.emplace_back(
      slice(var, dim, indices.back(), var.dimensions().size(dim)));
  return vars;
}

Variable concatenate(const Variable &a1, const Variable &a2,
                     const Dimension dim) {
  if (a1.type() != a2.type())
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
      if (dims2.size(dim1) != dims1.size(dim1))
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
    extent1 += dims1.size(dim) - 1;
  if (dims2.contains(dim))
    extent2 += dims2.size(dim) - 1;
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
  const Dim dim = coordDimension[newCoord.type()];
  dims.resize(dim, newCoord.dimensions().size(dim) - 1);
  rebinned.setDimensions(dims);
  // TODO take into account unit if values have been divided by bin width.
  rebinned.data().rebin(var.data(), dim, oldCoord.data(), newCoord.data());
  return rebinned;
}

Variable permute(const Variable &var, const Dimension dim,
                 const std::vector<gsl::index> &indices) {
  auto permuted(var);
  for (gsl::index i = 0; i < indices.size(); ++i)
    permuted.data().copy(var.data(), dim, i, indices[i], indices[i] + 1);
  return permuted;
}

Variable filter(const Variable &var, const Variable &filter) {
  if (filter.dimensions().ndim() != 1)
    throw std::runtime_error(
        "Cannot filter variable: The filter must by 1-dimensional.");
  const auto dim = filter.dimensions().labels()[0];
  auto mask = filter.get<const Coord::Mask>();

  const gsl::index removed = std::count(mask.begin(), mask.end(), 0);
  if (removed == 0)
    return var;

  auto out(var);
  auto dims = out.dimensions();
  dims.resize(dim, dims.size(dim) - removed);
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
