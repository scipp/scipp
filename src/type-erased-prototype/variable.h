#ifndef VARIABLE_H
#define VARIABLE_H

#include <string>
#include <type_traits>

#include <gsl/gsl_util>
#include <gsl/span>

#include "cow_ptr.h"
#include "dimensions.h"
#include "tags.h"
#include "unit.h"
#include "variable_view.h"

class VariableConcept {
public:
  virtual ~VariableConcept() = default;
  virtual std::unique_ptr<VariableConcept> clone() const = 0;
  virtual bool operator==(const VariableConcept &other) const = 0;
  virtual VariableConcept &operator+=(const VariableConcept &other) = 0;
  virtual gsl::index size() const = 0;
  virtual void resize(const gsl::index) = 0;
  virtual void copyFrom(const gsl::index chunkSize, const gsl::index chunkStart,
                        const gsl::index chunkSkip,
                        const VariableConcept &other) = 0;

  virtual const Dimensions &dimensions() const = 0;
  virtual void setDimensions(const Dimensions &dimensions) = 0;
};

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

class Variable {
public:
  template <class T>
  Variable(uint32_t id, Dimensions dimensions, T object)
      : m_type(id), m_unit{Unit::Id::Dimensionless},
        m_object(std::make_unique<VariableModel<T>>(std::move(dimensions),
                                                    std::move(object))) {}

  const std::string &name() const { return m_name; }
  void setName(const std::string &name) {
    if (isCoord())
      throw std::runtime_error("Coordinate variable cannot have a name.");
    m_name = name;
  }
  bool operator==(const Variable &other) const;
  Variable &operator+=(const Variable &other);

  const Unit &unit() const { return m_unit; }
  void setUnit(const Unit &unit) {
    // TODO
    // Some variables are special, e.g., Data::Tof, which must always have a
    // time-of-flight-related unit. We need some sort of check here. Is there a
    // better mechanism to implement this that does not require gatekeeping here
    // but expresses itself on the interface instead? Does it make sense to
    // handle all unit changes by conversion functions?
    m_unit = unit;
  }

  gsl::index size() const { return m_object->size(); }

  const Dimensions &dimensions() const { return m_object->dimensions(); }
  void setDimensions(const Dimensions &dimensions) {
    m_object.access().setDimensions(dimensions);
  }

  const VariableConcept &data() const { return *m_object; }
  VariableConcept &data() { return m_object.access(); }

  template <class Tag> bool valueTypeIs() const {
    return tag_id<Tag> == m_type;
  }

  uint16_t type() const { return m_type; }
  bool isCoord() const { return m_type < std::tuple_size<Coord::tags>::value; }

  template <class Tag> auto get() const {
    // For now we support only variables that are a std::vector. In principle we
    // could support anything that is convertible to gsl::span (or an adequate
    // replacement).
    return gsl::make_span(cast<std::vector<typename Tag::type>>());
  }

  template <class Tag>
  auto get(std::enable_if_t<std::is_const<Tag>::value> * = nullptr) {
    return const_cast<const Variable *>(this)->get<Tag>();
  }

  template <class Tag>
  auto get(std::enable_if_t<!std::is_const<Tag>::value> * = nullptr) {
    return gsl::make_span(cast<std::vector<typename Tag::type>>());
  }

private:
  template <class T> const T &cast() const {
    return dynamic_cast<const VariableModel<T> &>(*m_object).m_model;
  }

  template <class T> T &cast() {
    return dynamic_cast<VariableModel<T> &>(m_object.access()).m_model;
  }

  uint16_t m_type;
  std::string m_name;
  Unit m_unit;
  cow_ptr<VariableConcept> m_object;
};

template <class Tag, class... Args>
Variable makeVariable(Dimensions dimensions, Args &&... args) {
  return Variable(tag_id<Tag>, std::move(dimensions),
                  std::vector<typename Tag::type>(std::forward<Args>(args)...));
}

template <class Tag, class T>
Variable makeVariable(Dimensions dimensions, std::initializer_list<T> values) {
  return Variable(tag_id<Tag>, std::move(dimensions),
                  std::vector<typename Tag::type>(values));
}

Variable concatenate(const Dimension dim, const Variable &a1,
                     const Variable &a2);

#endif // VARIABLE_H
