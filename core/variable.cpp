/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <cmath>

#include "counts.h"
#include "dataset.h"
#include "except.h"
#include "variable.h"
#include "variable_view.h"

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
    const auto &oldData = oldT.getSpan();
    auto newData = newT.getSpan();
    const auto oldSize = oldT.dimensions()[dim];
    const auto newSize = newT.dimensions()[dim];
    const auto count = oldT.dimensions().volume() / oldSize;
    const auto *xold = &*oldCoordT.getSpan().begin();
    const auto *xnew = &*newCoordT.getSpan().begin();
    // This function assumes that dimensions between coord and data either
    // match, or coord is 1D.
    const bool jointOld = oldCoordT.dimensions().ndim() == 1;
    const bool jointNew = newCoordT.dimensions().ndim() == 1;
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
    const auto oldSize = oldT.dimensions()[dim];
    const auto newSize = newT.dimensions()[dim];

    const auto *xold = oldCoordT.span<T>().data();
    const auto *xnew = newCoordT.span<T>().data();
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
        newT(dim, inew) += oldT(dim, iold) * delta / owidth;
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
    : m_dimensions(dimensions){};

bool isMatchingOr1DBinEdge(const Dim dim, Dimensions edges,
                           const Dimensions &toMatch) {
  if (edges.ndim() == 1)
    return true;
  edges.resize(dim, edges[dim] - 1);
  return edges == toMatch;
}

template <class T>
auto makeSpan(T &model, const Dimensions &dims, const Dim dim,
              const scipp::index begin, const scipp::index end) {
  if (!dims.contains(dim) && (begin != 0 || end != 1))
    throw std::runtime_error("VariableConcept: Slice index out of range.");
  if (!dims.contains(dim) || dims[dim] == end - begin) {
    return scipp::span(model.data(), model.data() + model.size());
  }
  const scipp::index beginOffset = begin * dims.offset(dim);
  const scipp::index endOffset = end * dims.offset(dim);
  return scipp::span(model.data() + beginOffset, model.data() + endOffset);
}

template <class T> VariableConceptHandle VariableConceptT<T>::makeView() const {
  auto &dims = this->dimensions();
  return std::make_unique<ViewModel<decltype(getView(dims))>>(dims,
                                                              getView(dims));
}

template <class T> VariableConceptHandle VariableConceptT<T>::makeView() {
  if (this->isConstView())
    return const_cast<const VariableConceptT &>(*this).makeView();
  auto &dims = this->dimensions();
  return std::make_unique<ViewModel<decltype(getView(dims))>>(dims,
                                                              getView(dims));
}

template <class T>
VariableConceptHandle
VariableConceptT<T>::makeView(const Dim dim, const scipp::index begin,
                              const scipp::index end) const {
  auto dims = this->dimensions();
  if (end == -1)
    dims.erase(dim);
  else
    dims.resize(dim, end - begin);
  return std::make_unique<ViewModel<decltype(getView(dims, dim, begin))>>(
      dims, getView(dims, dim, begin));
}

template <class T>
VariableConceptHandle VariableConceptT<T>::makeView(const Dim dim,
                                                    const scipp::index begin,
                                                    const scipp::index end) {
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

template <class T>
VariableConceptHandle
VariableConceptT<T>::reshape(const Dimensions &dims) const {
  if (this->dimensions().volume() != dims.volume())
    throw std::runtime_error(
        "Cannot reshape to dimensions with different volume");
  return std::make_unique<ViewModel<decltype(getReshaped(dims))>>(
      dims, getReshaped(dims));
}

template <class T>
VariableConceptHandle VariableConceptT<T>::reshape(const Dimensions &dims) {
  if (this->dimensions().volume() != dims.volume())
    throw std::runtime_error(
        "Cannot reshape to dimensions with different volume");
  return std::make_unique<ViewModel<decltype(getReshaped(dims))>>(
      dims, getReshaped(dims));
}

template <class T>
bool VariableConceptT<T>::operator==(const VariableConcept &other) const {
  const auto &dims = this->dimensions();
  if (dims != other.dimensions())
    return false;
  if (this->dtype() != other.dtype())
    return false;
  const auto &otherT = requireT<const VariableConceptT>(other);
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

template <class T>
void VariableConceptT<T>::copy(const VariableConcept &other, const Dim dim,
                               const scipp::index offset,
                               const scipp::index otherBegin,
                               const scipp::index otherEnd) {
  auto iterDims = this->dimensions();
  const scipp::index delta = otherEnd - otherBegin;
  if (iterDims.contains(dim))
    iterDims.resize(dim, delta);

  const auto &otherT = requireT<const VariableConceptT>(other);
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

/// Implementation of VariableConcept that holds data.
template <class T> class DataModel : public conceptT_t<typename T::value_type> {
public:
  using value_type = std::remove_const_t<typename T::value_type>;

  DataModel(const Dimensions &dimensions, T model)
      : conceptT_t<typename T::value_type>(std::move(dimensions)),
        m_model(std::move(model)) {
    if (this->dimensions().volume() != scipp::size(m_model))
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
  }

  scipp::span<value_type> getSpan() override {
    return scipp::span(m_model.data(), m_model.data() + size());
  }
  scipp::span<value_type> getSpan(const Dim dim, const scipp::index begin,
                                  const scipp::index end) override {
    return makeSpan(m_model, this->dimensions(), dim, begin, end);
  }

  scipp::span<const value_type> getSpan() const override {
    return scipp::span(m_model.data(), m_model.data() + size());
  }
  scipp::span<const value_type> getSpan(const Dim dim, const scipp::index begin,
                                        const scipp::index end) const override {
    return makeSpan(m_model, this->dimensions(), dim, begin, end);
  }

  VariableView<value_type> getView(const Dimensions &dims) override {
    return makeVariableView(m_model.data(), 0, dims, this->dimensions());
  }
  VariableView<value_type> getView(const Dimensions &dims, const Dim dim,
                                   const scipp::index begin) override {
    scipp::index beginOffset = this->dimensions().contains(dim)
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
          const scipp::index begin) const override {
    scipp::index beginOffset = this->dimensions().contains(dim)
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

  VariableConceptHandle clone() const override {
    return std::make_unique<DataModel<T>>(this->dimensions(), m_model);
  }

  VariableConceptHandle clone(const Dimensions &dims) const override {
    return std::make_unique<DataModel<T>>(dims, T(dims.volume()));
  }

  bool isContiguous() const override { return true; }
  bool isView() const override { return false; }
  bool isConstView() const override { return false; }

  scipp::index size() const override { return m_model.size(); }

  T m_model;
};

namespace detail {
template <class T>
std::unique_ptr<VariableConceptT<T>>
makeVariableConceptT(const Dimensions &dims) {
  return std::make_unique<DataModel<Vector<T>>>(dims, Vector<T>(dims.volume()));
}
template <class T>
std::unique_ptr<VariableConceptT<T>>
makeVariableConceptT(const Dimensions &dims, Vector<T> data) {
  return std::make_unique<DataModel<Vector<T>>>(dims, std::move(data));
}
template std::unique_ptr<VariableConceptT<double>>
makeVariableConceptT<double>(const Dimensions &);
template std::unique_ptr<VariableConceptT<float>>
makeVariableConceptT<float>(const Dimensions &);
template std::unique_ptr<VariableConceptT<double>>
makeVariableConceptT<double>(const Dimensions &, Vector<double>);
template std::unique_ptr<VariableConceptT<float>>
makeVariableConceptT<float>(const Dimensions &, Vector<float>);
} // namespace detail

/// Implementation of VariableConcept that represents a view onto data.
template <class T>
class ViewModel
    : public conceptT_t<std::remove_const_t<typename T::element_type>> {
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

  ViewModel(const Dimensions &dimensions, T model)
      : conceptT_t<value_type>(std::move(dimensions)),
        m_model(std::move(model)) {
    if (this->dimensions().volume() != m_model.size())
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
  }

  scipp::span<value_type> getSpan() override {
    requireMutable();
    requireContiguous();
    if constexpr (std::is_const<typename T::element_type>::value)
      return scipp::span<value_type>();
    else
      return scipp::span(m_model.data(), m_model.data() + size());
  }
  scipp::span<value_type> getSpan(const Dim dim, const scipp::index begin,
                                  const scipp::index end) override {
    requireMutable();
    requireContiguous();
    if constexpr (std::is_const<typename T::element_type>::value) {
      static_cast<void>(dim);
      static_cast<void>(begin);
      static_cast<void>(end);
      return scipp::span<value_type>();
    } else {
      return makeSpan(m_model, this->dimensions(), dim, begin, end);
    }
  }

  scipp::span<const value_type> getSpan() const override {
    requireContiguous();
    return scipp::span(m_model.data(), m_model.data() + size());
  }
  scipp::span<const value_type> getSpan(const Dim dim, const scipp::index begin,
                                        const scipp::index end) const override {
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
                                   const scipp::index begin) override {
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
          const scipp::index begin) const override {
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

  VariableConceptHandle clone() const override {
    return std::make_unique<ViewModel<T>>(this->dimensions(), m_model);
  }

  VariableConceptHandle clone(const Dimensions &) const override {
    throw std::runtime_error("Cannot resize view.");
  }

  bool isContiguous() const override {
    return this->dimensions().isContiguousIn(m_model.parentDimensions());
  }
  bool isView() const override { return true; }
  bool isConstView() const override {
    return std::is_const<typename T::element_type>::value;
  }

  scipp::index size() const override { return m_model.size(); }

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
Variable::Variable(const Variable &parent, const Dimensions &dims)
    : m_tag(parent.tag()), m_unit(parent.unit()), m_name(parent.m_name),
      m_object(parent.m_object->clone(dims)) {}

Variable::Variable(const ConstVariableSlice &parent, const Dimensions &dims)
    : m_tag(parent.tag()), m_unit(parent.unit()),
      m_object(parent.data().clone(dims)) {
  setName(parent.name());
}

Variable::Variable(const Variable &parent, VariableConceptHandle data)
    : m_tag(parent.tag()), m_unit(parent.unit()), m_name(parent.m_name),
      m_object(std::move(data)) {}

template <class T>
Variable::Variable(const Tag tag, const units::Unit unit,
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

template <class T> const Vector<underlying_type_t<T>> &Variable::cast() const {
  return requireT<const DataModel<Vector<underlying_type_t<T>>>>(*m_object)
      .m_model;
}

template <class T> Vector<underlying_type_t<T>> &Variable::cast() {
  return requireT<DataModel<Vector<underlying_type_t<T>>>>(*m_object).m_model;
}

#define INSTANTIATE(...)                                                       \
  template Variable::Variable(const Tag, const units::Unit,                    \
                              const Dimensions &,                              \
                              Vector<underlying_type_t<__VA_ARGS__>>);         \
  template Vector<underlying_type_t<__VA_ARGS__>>                              \
      &Variable::cast<__VA_ARGS__>();                                          \
  template const Vector<underlying_type_t<__VA_ARGS__>>                        \
      &Variable::cast<__VA_ARGS__>() const;

INSTANTIATE(std::string)
INSTANTIATE(double)
INSTANTIATE(float)
INSTANTIATE(int64_t)
INSTANTIATE(int32_t)
INSTANTIATE(char)
INSTANTIATE(bool)
INSTANTIATE(std::pair<int64_t, int64_t>)
INSTANTIATE(ValueWithDelta<double>)
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
INSTANTIATE(scipp::index)
INSTANTIATE(std::pair<scipp::index, scipp::index>)
#endif
INSTANTIATE(boost::container::small_vector<scipp::index, 1>)
INSTANTIATE(boost::container::small_vector<double, 8>)
INSTANTIATE(std::vector<double>)
INSTANTIATE(std::vector<std::string>)
INSTANTIATE(std::vector<scipp::index>)
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
  return equals(*this, other);
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

template <class T1, class T2> T1 &plus_equals(T1 &variable, const T2 &other) {
  // Addition with different Variable type is supported, mismatch of underlying
  // element types is handled in DataModel::operator+=.
  // Different name is ok for addition.
  expect::equals(variable.unit(), other.unit());
  // TODO How should attributes be handled?
  if (variable.dtype() != dtype<Dataset> || variable.isAttr()) {
    expect::contains(variable.dimensions(), other.dimensions());
    // Note: This will broadcast/transpose the RHS if required. We do not
    // support changing the dimensions of the LHS though!
    variable.template transform_in_place<
        pair_self_t<double, float, int64_t, Eigen::Vector3d>>(
        [](auto &&a, auto &&b) { return a + b; }, other);
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
      for (scipp::index i = 0; i < scipp::size(datasets); ++i)
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
  expect::equals(variable.unit(), other.unit());
  expect::contains(variable.dimensions(), other.dimensions());
  if (variable.tag() == Data::Events)
    throw std::runtime_error("Subtraction of events lists not implemented.");
  variable.template transform_in_place<
      pair_self_t<double, float, int64_t, Eigen::Vector3d>>(
      [](auto &&a, auto &&b) { return a - b; }, other);
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
  expect::contains(variable.dimensions(), other.dimensions());
  if (variable.tag() == Data::Events)
    throw std::runtime_error("Multiplication of events lists not implemented.");
  // setUnit is catching bad cases of changing units (if `variable` is a slice).
  variable.setUnit(variable.unit() * other.unit());
  variable.template transform_in_place<
      pair_self_t<double, float, int64_t>,
      pair_custom_t<std::pair<Eigen::Vector3d, double>>>(
      [](auto &&a, auto &&b) { return a * b; }, other);
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
  other.setUnit(units::dimensionless);
  return times_equals(*this, other);
}

template <class T1, class T2> T1 &divide_equals(T1 &variable, const T2 &other) {
  expect::contains(variable.dimensions(), other.dimensions());
  if (variable.tag() == Data::Events)
    throw std::runtime_error("Division of events lists not implemented.");
  // setUnit is catching bad cases of changing units (if `variable` is a slice).
  variable.setUnit(variable.unit() / other.unit());
  variable.template transform_in_place<
      pair_self_t<double, float, int64_t>,
      pair_custom_t<std::pair<Eigen::Vector3d, double>>>(
      [](auto &&a, auto &&b) { return a / b; }, other);
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
  other.setUnit(units::dimensionless);
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
    throw except::DimensionMismatchError(dimensions(), other.dimensions());
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
  other.setUnit(units::dimensionless);
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
  other.setUnit(units::dimensionless);
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
  other.setUnit(units::dimensionless);
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
  other.setUnit(units::dimensionless);
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

void VariableSlice::setUnit(const units::Unit &unit) const {
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
const VariableView<const underlying_type_t<T>>
ConstVariableSlice::cast() const {
  using TT = underlying_type_t<T>;
  if (!m_view)
    return requireT<const DataModel<Vector<TT>>>(data()).getView(dimensions());
  if (m_view->isConstView())
    return requireT<const ViewModel<VariableView<const TT>>>(data()).m_model;
  // Make a const view from the mutable one.
  return {requireT<const ViewModel<VariableView<TT>>>(data()).m_model,
          dimensions()};
}

template <class T>
VariableView<underlying_type_t<T>> VariableSlice::cast() const {
  using TT = underlying_type_t<T>;
  if (m_view)
    return requireT<const ViewModel<VariableView<TT>>>(data()).m_model;
  return requireT<DataModel<Vector<TT>>>(data()).getView(dimensions());
}

#define INSTANTIATE_SLICEVIEW(...)                                             \
  template const VariableView<const underlying_type_t<__VA_ARGS__>>            \
  ConstVariableSlice::cast<__VA_ARGS__>() const;                               \
  template VariableView<underlying_type_t<__VA_ARGS__>>                        \
  VariableSlice::cast<__VA_ARGS__>() const;

INSTANTIATE_SLICEVIEW(double);
INSTANTIATE_SLICEVIEW(float);
INSTANTIATE_SLICEVIEW(int64_t);
INSTANTIATE_SLICEVIEW(int32_t);
INSTANTIATE_SLICEVIEW(char);
INSTANTIATE_SLICEVIEW(bool);
INSTANTIATE_SLICEVIEW(std::string);
INSTANTIATE_SLICEVIEW(boost::container::small_vector<double, 8>);
INSTANTIATE_SLICEVIEW(Dataset);
INSTANTIATE_SLICEVIEW(Eigen::Vector3d);

ConstVariableSlice Variable::operator()(const Dim dim, const scipp::index begin,
                                        const scipp::index end) const & {
  return {*this, dim, begin, end};
}

VariableSlice Variable::operator()(const Dim dim, const scipp::index begin,
                                   const scipp::index end) & {
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
Variable operator+(Variable a, const Variable &b) {
  auto result = broadcast(std::move(a), b.dimensions());
  return result += b;
}
Variable operator-(Variable a, const Variable &b) {
  auto result = broadcast(std::move(a), b.dimensions());
  return result -= b;
}
Variable operator*(Variable a, const Variable &b) {
  auto result = broadcast(std::move(a), b.dimensions());
  return result *= b;
}
Variable operator/(Variable a, const Variable &b) {
  auto result = broadcast(std::move(a), b.dimensions());
  return result /= b;
}
Variable operator+(Variable a, const ConstVariableSlice &b) {
  auto result = broadcast(std::move(a), b.dimensions());
  return result += b;
}
Variable operator-(Variable a, const ConstVariableSlice &b) {
  auto result = broadcast(std::move(a), b.dimensions());
  return result -= b;
}
Variable operator*(Variable a, const ConstVariableSlice &b) {
  auto result = broadcast(std::move(a), b.dimensions());
  return result *= b;
}
Variable operator/(Variable a, const ConstVariableSlice &b) {
  auto result = broadcast(std::move(a), b.dimensions());
  return result /= b;
}
Variable operator+(Variable a, const double b) { return std::move(a += b); }
Variable operator-(Variable a, const double b) { return std::move(a -= b); }
Variable operator*(Variable a, const double b) { return std::move(a *= b); }
Variable operator/(Variable a, const double b) { return std::move(a /= b); }
Variable operator+(const double a, Variable b) { return std::move(b += a); }
Variable operator-(const double a, Variable b) { return -(b -= a); }
Variable operator*(const double a, Variable b) { return std::move(b *= a); }
Variable operator/(const double a, Variable b) {
  b.setUnit(units::Unit(units::dimensionless) / b.unit());
  b.transform_in_place<double, float>(
      overloaded{[a](const double b) { return a / b; },
                 [a](const float b) { return a / b; }});
  return std::move(b);
}

// Example of a "derived" operation: Implementation does not require adding a
// virtual function to VariableConcept.
std::vector<Variable> split(const Variable &var, const Dim dim,
                            const std::vector<scipp::index> &indices) {
  if (indices.empty())
    return {var};
  std::vector<Variable> vars;
  vars.emplace_back(var(dim, 0, indices.front()));
  for (scipp::index i = 0; i < scipp::size(indices) - 1; ++i)
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
  out.setDimensions(dims);

  out.data().copy(a1.data(), dim, 0, 0, extent1);
  out.data().copy(a2.data(), dim, extent1, 0, extent2);

  return out;
}

Variable rebin(const Variable &var, const Variable &oldCoord,
               const Variable &newCoord) {

  expect::countsOrCountsDensity(var);
  const Dim dim = coordDimension[newCoord.tag().value()];

  auto do_rebin = [dim](auto &&out, auto &&old, auto &&oldCoord,
                        auto &&newCoord) {
    // Dimensions of *this and old are guaranteed to be the same.
    const auto &oldT = *old;
    const auto &oldCoordT = *oldCoord;
    const auto &newCoordT = *newCoord;
    auto &outT = *out;
    const auto &dims = outT.dimensions();
    if (dims.inner() == dim &&
        isMatchingOr1DBinEdge(dim, oldCoordT.dimensions(), oldT.dimensions()) &&
        isMatchingOr1DBinEdge(dim, newCoordT.dimensions(), dims)) {
      RebinHelper<typename std::remove_reference_t<decltype(
          outT)>::value_type>::rebinInner(dim, oldT, outT, oldCoordT,
                                          newCoordT);
    } else {
      throw std::runtime_error(
          "TODO the new coord should be 1D or the same dim as newCoord.");
    }
  };

  if (var.unit() == units::counts ||
      var.unit() == units::counts * units::counts) {
    auto dims = var.dimensions();
    dims.resize(dim, newCoord.dimensions()[dim] - 1);
    Variable rebinned(var, dims);
    if (rebinned.dimensions().inner() == dim) {
      rebinned.apply_in_place<double, float>(do_rebin, var, oldCoord, newCoord);
    } else {
      if (newCoord.dimensions().ndim() > 1)
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
    Dataset density;
    density.insert(oldCoord);
    density.insert(var);
    auto cnts = counts::fromDensity(std::move(density), dim)
                    .erase(var.tag(), var.name());
    Dataset rebinnedCounts;
    rebinnedCounts.insert(newCoord);
    rebinnedCounts.insert(rebin(cnts, oldCoord, newCoord));
    return counts::toDensity(std::move(rebinnedCounts), dim)
        .erase(var.tag(), var.name());
  }
}

Variable permute(const Variable &var, const Dim dim,
                 const std::vector<scipp::index> &indices) {
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
  auto mask = filter.span<bool>();

  const scipp::index removed = std::count(mask.begin(), mask.end(), 0);
  if (removed == 0)
    return var;

  auto out(var);
  auto dims = out.dimensions();
  dims.resize(dim, dims[dim] - removed);
  out.setDimensions(dims);

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
  auto dims = summed.dimensions();
  dims.erase(dim);
  // setDimensions zeros the data
  summed.setDimensions(dims);
  summed.template transform_in_place<
      pair_self_t<double, float, int64_t, Eigen::Vector3d>>(
      [](auto &&a, auto &&b) { return a + b; }, var);
  return summed;
}

Variable mean(const Variable &var, const Dim dim) {
  auto summed = sum(var, dim);
  double scale = 1.0 / static_cast<double>(var.dimensions()[dim]);
  return summed * Variable(Data::Value, {}, {scale});
}

Variable abs(const Variable &var) {
  return var.transform<double, float>([](const auto x) { return ::abs(x); });
}

Variable norm(const Variable &var) {
  return var.transform<Eigen::Vector3d>([](auto &&x) { return x.norm(); });
}

Variable sqrt(const Variable &var) {
  Variable result =
      var.transform<double, float>([](const auto x) { return std::sqrt(x); });
  result.setUnit(sqrt(var.unit()));
  return result;
}

Variable broadcast(Variable var, const Dimensions &dims) {
  if (var.dimensions().contains(dims))
    return std::move(var);
  auto newDims = var.dimensions();
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
  result.setDimensions(newDims);
  result.data().copy(var.data(), Dim::Invalid, 0, 0, 1);
  return result;
}

void swap(Variable &var, const Dim dim, const scipp::index a,
          const scipp::index b) {
  const Variable tmp = var(dim, a);
  var(dim, a).assign(var(dim, b));
  var(dim, b).assign(tmp);
}

Variable reverse(Variable var, const Dim dim) {
  const auto size = var.dimensions()[dim];
  for (scipp::index i = 0; i < size / 2; ++i)
    swap(var, dim, i, size - i - 1);
  return std::move(var);
}

template <>
VariableView<const double> getView<double>(const Variable &var,
                                           const Dimensions &dims) {
  return requireT<const VariableConceptT<double>>(var.data()).getView(dims);
}

} // namespace scipp::core
