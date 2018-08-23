#include "variable.h"
#include "variable_view.h"

template <class T> struct ArithmeticHelper {
  static void plus_equals(std::vector<T> &a,
                          const VariableView<std::vector<T>> &b) {
    std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<T>());
  }
};

template <class T> struct ArithmeticHelper<std::vector<T>> {
  static void plus_equals(std::vector<std::vector<T>> &a,
                          const VariableView<std::vector<std::vector<T>>> &b) {
    throw std::runtime_error("Not an arithmetic type. Addition not possible.");
  }
};

template <> struct ArithmeticHelper<std::string> {
  static void plus_equals(std::vector<std::string> &a,
                          const VariableView<std::vector<std::string>> &b) {
    throw std::runtime_error("Cannot add strings. Use append() instead.");
  }
};

template <class T> class VariableModel : public VariableConcept {
public:
  VariableModel(Dimensions dimensions, T model)
      : m_dimensions(std::move(dimensions)), m_model(std::move(model)) {
    if (m_dimensions.volume() != m_model.size())
      throw std::runtime_error("Creating Variable: data size does not match "
                               "volume given by dimension extents");
  }

  std::unique_ptr<VariableConcept> clone() const override {
    return std::make_unique<VariableModel<T>>(m_dimensions, m_model);
  }

  bool operator==(const VariableConcept &other) const override {
    return m_model == dynamic_cast<const VariableModel<T> &>(other).m_model;
  }

  VariableConcept &operator+=(const VariableConcept &other) override {
    try {
      ArithmeticHelper<typename T::value_type>::plus_equals(
          m_model,
          VariableView<T>(dynamic_cast<const VariableModel<T> &>(other).m_model,
                          dimensions(), other.dimensions()));
    } catch (const std::bad_cast &) {
      throw std::runtime_error(
          "Cannot add Variables: Underlying data types do not match.");
    }
    return *this;
  }

  gsl::index size() const override { return m_model.size(); }
  void resize(const gsl::index size) override { m_model.resize(size); }

  void copyFrom(const gsl::index chunkSize, const gsl::index chunkStart,
                const gsl::index chunkSkip,
                const VariableConcept &other) override {
    const auto &source = dynamic_cast<const VariableModel<T> &>(other);
    auto in = source.m_model.begin();
    auto in_end = source.m_model.end();
    auto out = m_model.begin() + chunkStart * chunkSize;
    auto out_end = m_model.end();
    while (in != in_end && out != out_end) {
      for (gsl::index i = 0; i < chunkSize; ++i) {
        *out = *in;
        ++out;
        ++in;
      }
      out += (chunkSkip - 1) * chunkSize;
    }
  }

  const Dimensions &dimensions() const override { return m_dimensions; }
  void setDimensions(const Dimensions &dimensions) override {
    // TODO Zero data? Or guarentee that equivalent data is moved to correct
    // target position?
    m_dimensions = dimensions;
    resize(m_dimensions.volume());
  }

  Dimensions m_dimensions;
  T m_model;
};

template <class T>
Variable::Variable(uint32_t id, Dimensions dimensions, T object)
    : m_type(id), m_unit{Unit::Id::Dimensionless},
      m_object(std::make_unique<VariableModel<T>>(std::move(dimensions),
                                                  std::move(object))) {}

template <class T> const T &Variable::cast() const {
  return dynamic_cast<const VariableModel<T> &>(*m_object).m_model;
}

template <class T> T &Variable::cast() {
  return dynamic_cast<VariableModel<T> &>(m_object.access()).m_model;
}

#define INSTANTIATE(type)                                                      \
  template Variable::Variable(uint32_t, Dimensions, std::vector<type>);        \
  template std::vector<type> &Variable::cast<std::vector<type>>();             \
  template const std::vector<type> &Variable::cast<std::vector<type>>() const;

INSTANTIATE(std::string)
INSTANTIATE(double)
INSTANTIATE(int32_t)
INSTANTIATE(int64_t)
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
  if (!(dims1 == dims2))
    throw std::runtime_error(
        "Cannot concatenate Variables: Dimensions do not match.");
  // Should we permit creation of ragged outputs if one dimension does not
  // match?
  auto out(a1);
  auto dims(dims1);
  if (dims.contains(dim)) {
    dims.resize(dim, dims1.size(dim) + dims2.size(dim));
    out.setDimensions(dims);
    auto offset = dims1.offset(dim) * dims1.size(dim);
    out.data().copyFrom(offset, 0, 2, a1.data());
    out.data().copyFrom(offset, 1, 2, a2.data());
  } else {
    dims.add(dim, 2);
    out.setDimensions(dims);
    out.data().copyFrom(a1.size(), 1, 2, a2.data());
  }
  return out;
}
