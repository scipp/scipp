/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "variable.h"
#include "variable_view.h"

template <template <class> class Op, class T> struct ArithmeticHelper {
  static void apply(Vector<T> &a, const VariableView<const Vector<T>> &b) {
    std::transform(a.begin(), a.end(), b.begin(), a.begin(), Op<T>());
  }
  static void apply(Vector<T> &a, const Vector<T> &b) {
    std::transform(a.begin(), a.end(), b.begin(), a.begin(), Op<T>());
  }
};

template <template <class> class Op, class T>
struct ArithmeticHelper<Op, std::vector<T>> {
  template <class Other>
  static void apply(Vector<std::vector<T>> &a, const Other &b) {
    throw std::runtime_error("Not an arithmetic type. Cannot apply operand.");
  }
};

template <template <class> class Op, class T>
struct ArithmeticHelper<Op, std::pair<T, T>> {
  template <class Other>
  static void apply(Vector<std::pair<T, T>> &a, const Other &b) {
    throw std::runtime_error("Not an arithmetic type. Cannot apply operand.");
  }
};

template <template <class> class Op> struct ArithmeticHelper<Op, std::string> {
  template <class Other>
  static void apply(Vector<std::string> &a, const Other) {
    throw std::runtime_error("Cannot add strings. Use append() instead.");
  }
};

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
  VariableModel(Dimensions dimensions, T model)
      : VariableConcept(std::move(dimensions)), m_model(std::move(model)) {
    if (this->dimensions().volume() != m_model.size())
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
  }

  std::unique_ptr<VariableConcept> clone() const override {
    return std::make_unique<VariableModel<T>>(dimensions(), m_model);
  }

  std::unique_ptr<VariableConcept> cloneEmpty() const override {
    return std::make_unique<VariableModel<T>>(Dimensions{}, T(1));
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
  void resize(const gsl::index size) override { m_model.resize(size); }

  void copySlice(const VariableConcept &otherConcept, const Dimension dim,
                 const gsl::index index) override {
    const auto &other = dynamic_cast<const VariableModel<T> &>(otherConcept);
    auto data = gsl::make_span(other.m_model.data() +
                                   index * other.dimensions().offset(dim),
                               &*other.m_model.end());
    auto sliceDims = other.dimensions();
    if (index >= sliceDims.size(dim) || index < 0)
      throw std::runtime_error("Slice index out of range");
    if (sliceDims.label(sliceDims.count() - 1) == dim) {
      // Slicing slowest dimension so data is contiguous, avoid using view.
      std::copy(data.begin(), data.begin() + m_model.size(), m_model.begin());
    } else {
      sliceDims.erase(dim);
      VariableView<const decltype(data)> sliceView(data, sliceDims,
                                                   other.dimensions());
      std::copy(sliceView.begin(), sliceView.end(), m_model.begin());
    }
  }

  void copyFrom(const VariableConcept &otherConcept, const Dimension dim,
                const gsl::index offset) override {
    // TODO Can probably merge this method with copySlice.
    const auto &other = dynamic_cast<const VariableModel<T> &>(otherConcept);

    auto iterationDimensions = dimensions();
    if (!other.dimensions().contains(dim))
      iterationDimensions.erase(dim);
    else
      iterationDimensions.resize(dim, other.dimensions().size(dim));

    auto target = gsl::make_span(
        m_model.data() + offset * dimensions().offset(dim), &*m_model.end());
    // For cases for minimizing use of VariableView --- just copy contiguous
    // range where possible.
    if (dimensions().label(dimensions().count() - 1) == dim) {
      if (iterationDimensions == other.dimensions()) {
        std::copy(other.m_model.begin(), other.m_model.end(), target.begin());
      } else {
        VariableView<const T> otherView(other.m_model, iterationDimensions,
                                        other.dimensions());
        std::copy(otherView.begin(), otherView.end(), target.begin());
      }
    } else {
      VariableView<decltype(target)> view(target, iterationDimensions,
                                          dimensions());
      if (iterationDimensions == other.dimensions()) {
        std::copy(other.m_model.begin(), other.m_model.end(), view.begin());
      } else {
        VariableView<const T> otherView(other.m_model, iterationDimensions,
                                        other.dimensions());
        std::copy(otherView.begin(), otherView.end(), view.begin());
      }
    }
  }

  T m_model;
};

template <class T>
Variable::Variable(uint32_t id, const Unit::Id unit, Dimensions dimensions,
                   T object)
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
  template Variable::Variable(uint32_t, const Unit::Id, Dimensions,            \
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
INSTANTIATE(std::vector<gsl::index>)

bool Variable::operator==(const Variable &other) const {
  // Compare even before pointer comparison since data may be shared even if
  // names differ.
  if (m_name != other.m_name)
    return false;
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
  if (dimensions().contains(other.dimensions())) {
    // Note: This will broadcast/transpose the RHS if required. We do not
    // support changing the dimensions of the LHS though!
    m_object.access() += *other.m_object;
  } else {
    throw std::runtime_error("Cannot add Variables: Dimensions do not match.");
  }

  return *this;
}

Variable &Variable::operator-=(const Variable &other) {
  if (m_unit != other.m_unit)
    throw std::runtime_error("Cannot subtract Variables: Units do not match.");
  if (dimensions().contains(other.dimensions())) {
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
  data().copyFrom(slice.data(), dim, index);
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
  out.data().copySlice(var.data(), dim, index);
  return out;
}

Variable concatenate(const Dimension dim, const Variable &a1,
                     const Variable &a2) {
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
  for (const auto &item : dims1) {
    if (item.first != dim) {
      if (!dims2.contains(item.first))
        throw std::runtime_error(
            "Cannot concatenate Variables: Dimensions do not match.");
      if (dims2.size(item.first) != item.second)
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

  // Should we permit creation of ragged outputs if one dimension does not
  // match?
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

  out.data().copyFrom(a1.data(), dim, 0);
  out.data().copyFrom(a2.data(), dim, extent1);

  return out;
}
