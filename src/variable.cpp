/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "variable.h"
#include "dataset.h"
#include "variable_view.h"

template <template <class> class Op, class T> struct ArithmeticHelper {
  static void apply(Vector<T> &a, const VariableView<const Vector<T>> &b) {
    std::transform(a.begin(), a.end(), b.begin(), a.begin(), Op<T>());
  }
  static void apply(Vector<T> &a, const Vector<T> &b) {
    std::transform(a.begin(), a.end(), b.begin(), a.begin(), Op<T>());
  }
  static void apply(const Vector<T> &a, const VariableView<const Vector<T>> &b,
                    Vector<T> &out) {
    std::transform(a.begin(), a.end(), b.begin(), out.begin(), Op<T>());
  }
  static void apply(const Vector<T> &a, const Vector<T> &b, Vector<T> &out) {
    std::transform(a.begin(), a.end(), b.begin(), out.begin(), Op<T>());
  }
};

template <class T> class VariableModel;
template <class T> struct RebinHelper {
  static void rebin(const Dim dim, const T &oldModel, T &newModel,
                    const VariableView<const T> &oldCoordView,
                    const gsl::index oldOffset,
                    const VariableView<const T> &newCoordView,
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

DISABLE_REBIN_T(std::shared_ptr<T>)
DISABLE_REBIN_T(std::array<T, 4>)
DISABLE_REBIN_T(std::array<T, 3>)
DISABLE_REBIN_T(std::vector<T>)
DISABLE_REBIN_T(boost::container::small_vector<T, 1>)
DISABLE_REBIN_T(std::pair<T, T>)
DISABLE_REBIN_T(ValueWithDelta<T>)
DISABLE_REBIN(Dataset)
DISABLE_REBIN(std::string)

VariableConcept::VariableConcept(const Dimensions &dimensions)
    : m_dimensions(dimensions){};

void VariableConcept::setDimensions(const Dimensions &dimensions) {
  // TODO Zero data? Or guarentee that equivalent data is moved to correct
  // target position?
  m_dimensions = dimensions;
  resize(m_dimensions.volume());
}

template <class T> class VariableModel final : public VariableConcept {
public:
  VariableModel(const Dimensions &dimensions)
      : VariableConcept(dimensions), m_model(this->dimensions().volume()) {}

  VariableModel(const Dimensions &dimensions, T model)
      : VariableConcept(std::move(dimensions)), m_model(std::move(model)) {
    if (this->dimensions().volume() != m_model.size())
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
  }

  std::shared_ptr<VariableConcept> clone() const override {
    return std::make_shared<VariableModel<T>>(dimensions(), m_model);
  }

  std::shared_ptr<VariableConcept> cloneEmpty() const override {
    return std::make_shared<VariableModel<T>>(Dimensions{}, T(1));
  }

  bool operator==(const VariableConcept &other) const override {
    return m_model == dynamic_cast<const VariableModel<T> &>(other).m_model;
  }

  template <template <class> class Op>
  VariableConcept &apply(const VariableConcept &other) {
    try {
      const auto &otherModel =
          dynamic_cast<const VariableModel<T> &>(other).m_model;
      if (dimensions() == other.dimensions()) {
        ArithmeticHelper<Op, typename T::value_type>::apply(m_model,
                                                            otherModel);
      } else {
        ArithmeticHelper<Op, typename T::value_type>::apply(
            m_model, VariableView<const T>(otherModel, dimensions(),
                                           other.dimensions()));
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
      const auto &oldCoordModel =
          dynamic_cast<const VariableModel<T> &>(oldCoord).m_model;
      const auto &newCoordModel =
          dynamic_cast<const VariableModel<T> &>(newCoord).m_model;
      auto oldCoordDims = oldCoord.dimensions();
      oldCoordDims.resize(dim, oldCoordDims.size(dim) - 1);
      auto newCoordDims = newCoord.dimensions();
      newCoordDims.resize(dim, newCoordDims.size(dim) - 1);
      VariableView<const T> oldCoordView(oldCoordModel, dimensions(),
                                         oldCoordDims);
      VariableView<const T> newCoordView(newCoordModel, dimensions(),
                                         newCoordDims);
      const auto oldOffset = oldCoordDims.offset(dim);
      const auto newOffset = newCoordDims.offset(dim);

      RebinHelper<T>::rebin(dim, oldModel, m_model, oldCoordView, oldOffset,
                            newCoordView, newOffset);
    }
  }

  template <template <class> class Op>
  std::unique_ptr<VariableConcept>
  applyBinary(const VariableConcept &other) const {
    auto result = std::make_unique<VariableModel<T>>(dimensions());
    auto &resultModel = result->m_model;
    try {
      // TODO We can use static_cast if the tag matches.
      const auto &otherModel =
          dynamic_cast<const VariableModel<T> &>(other).m_model;
      if (dimensions() == other.dimensions()) {
        ArithmeticHelper<Op, typename T::value_type>::apply(m_model, otherModel,
                                                            resultModel);
      } else {
        ArithmeticHelper<Op, typename T::value_type>::apply(
            m_model,
            VariableView<const T>(otherModel, dimensions(), other.dimensions()),
            resultModel);
      }
    } catch (const std::bad_cast &) {
      throw std::runtime_error("Cannot apply arithmetic operation to "
                               "Variables: Underlying data types do not "
                               "match.");
    }
    return result;
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

  std::unique_ptr<VariableConcept>
  operator+(const VariableConcept &other) const override {
    return applyBinary<std::plus>(other);
  }

  gsl::index size() const override { return m_model.size(); }
  void resize(const gsl::index size) override { m_model.resize(size); }

  void copy(const VariableConcept &otherConcept, const Dim dim,
            const gsl::index offset, const gsl::index otherBegin,
            const gsl::index otherEnd) override {
    const auto &other = dynamic_cast<const VariableModel<T> &>(otherConcept);

    auto iterationDimensions = dimensions();
    auto otherDims = other.dimensions();
    auto thisDims = dimensions();
    if (!iterationDimensions.contains(dim) && offset != 0)
      throw std::runtime_error("VariableConcept::copy: Output offset must be 0 "
                               "if dimension is not contained.");
    if (!other.dimensions().contains(dim)) {
      if (iterationDimensions.contains(dim))
        iterationDimensions.erase(dim);
      // Add fake dim such that we can use Dimensions::offset below.
      if (otherBegin != 0 || otherEnd != 1)
        throw std::runtime_error(
            "VariableConcept::copy: Slice index out of range.");
      otherDims.add(dim, 1);
    } else {
      if (iterationDimensions.contains(dim))
        iterationDimensions.resize(dim, otherEnd - otherBegin);
      else
        thisDims.add(dim, 1);
    }

    gsl::index otherBeginOffset = otherBegin * otherDims.offset(dim);
    gsl::index otherEndOffset = otherEnd * otherDims.offset(dim);
    auto target = gsl::make_span(m_model.data() + offset * thisDims.offset(dim),
                                 &*m_model.end());
    auto source = gsl::make_span(other.m_model.data() + otherBeginOffset,
                                 &*other.m_model.end());
    VariableView<const decltype(source)> otherView(source, iterationDimensions,
                                                   other.dimensions());
    // Four cases for minimizing use of VariableView --- just copy contiguous
    // range where possible.
    bool otherIsContiguous = otherDims.label(thisDims.count() - 1) == dim;
    if (thisDims.label(thisDims.count() - 1) == dim) {
      if (otherIsContiguous && iterationDimensions == other.dimensions()) {
        // Case 1: Output range is contiguous, input is contiguous and not
        // transposed.
        std::copy(source.begin(), source.begin() + otherEndOffset,
                  target.begin());
      } else {
        // Case 2: Output range is contiguous, input is not contiguous or
        // transposed.
        std::copy(otherView.begin(), otherView.end(), target.begin());
      }
    } else {
      VariableView<decltype(target)> view(target, iterationDimensions,
                                          dimensions());
      if (otherIsContiguous && iterationDimensions == other.dimensions()) {
        // Case 3: Output range is not contiguous, input is contiguous and not
        // transposed.
        std::copy(source.begin(), source.begin() + otherEndOffset,
                  view.begin());
      } else {
        // Case 4: Output range is not contiguous, input is not contiguous or
        // transposed.
        std::copy(otherView.begin(), otherView.end(), view.begin());
      }
    }
  }

  void copyPermute(const VariableConcept &otherConcept,
                   const std::vector<gsl::index> &indices) override {
    const auto &other =
        dynamic_cast<const VariableModel<T> &>(otherConcept).m_model;
    for (gsl::index i = 0; i < indices.size(); ++i) {
      m_model[i] = other[indices[i]];
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
  m_object = m_object->cloneEmpty();
  m_object.access().setDimensions(dimensions);
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
      if (m_object.unique()) {
        m_object.access() += *other.m_object;
      } else {
        // Handling special case to avoid copy should reduce data movement
        // from/to memory --- making copy requires 2 load + 1 store, `+=` is 2
        // load + 1 store, `+` is 3 load + 1 store. In practive I did not
        // observe a significant speedup though, need more benchmarks.
        m_object = *m_object + *other.m_object;
      }
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

Variable &Variable::operator-=(const Variable &other) {
  if (m_unit != other.m_unit)
    throw std::runtime_error("Cannot subtract Variables: Units do not match.");
  if (dimensions().contains(other.dimensions())) {
    if (valueTypeIs<Data::Events>())
      throw std::runtime_error("Subtraction of events lists not implemented.");
    m_object.access() -= *other.m_object;
  } else {
    throw std::runtime_error(
        "Cannot subtract Variables: Dimensions do not match.");
  }

  return *this;
}

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

Variable permute(const Variable &var, const std::vector<gsl::index> &indices) {
  auto permuted(var);
  permuted.data().copyPermute(var.data(), indices);
  return permuted;
}
