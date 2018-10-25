/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
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
#include "vector.h"

class VariableConcept {
public:
  VariableConcept(const Dimensions &dimensions);
  virtual ~VariableConcept() = default;
  // This is dropped into a cow_ptr so we prefer shared_ptr over unique_ptr.
  virtual std::shared_ptr<VariableConcept> clone() const = 0;
  virtual std::shared_ptr<VariableConcept> cloneEmpty() const = 0;
  virtual bool operator==(const VariableConcept &other) const = 0;
  virtual VariableConcept &operator+=(const VariableConcept &other) = 0;
  virtual VariableConcept &operator-=(const VariableConcept &other) = 0;
  virtual VariableConcept &operator*=(const VariableConcept &other) = 0;
  virtual std::unique_ptr<VariableConcept>
  operator+(const VariableConcept &other) const = 0;
  virtual gsl::index size() const = 0;
  virtual void resize(const gsl::index) = 0;
  virtual void copySlice(const VariableConcept &other, const Dimension dim,
                         const gsl::index index) = 0;
  virtual void copyFrom(const VariableConcept &other, const Dimension dim,
                        const gsl::index offset) = 0;

  const Dimensions &dimensions() const { return m_dimensions; }
  void setDimensions(const Dimensions &dimensions);

private:
  Dimensions m_dimensions;
};

template <class T> class deep_ptr {
public:
  deep_ptr() = default;
  deep_ptr(std::unique_ptr<T> &&other) : m_data(std::move(other)) {}
  deep_ptr(const deep_ptr<T> &other)
      : m_data(other ? std::make_unique<T>(*other) : nullptr) {}
  deep_ptr(deep_ptr<T> &&) = default;
  constexpr deep_ptr(std::nullptr_t){};
  deep_ptr<T> &operator=(const deep_ptr<T> &other) {
    if (&other != this && other)
      m_data = std::make_unique<T>(*other);
    return *this;
  }
  deep_ptr<T> &operator=(deep_ptr<T> &&) = default;

  explicit operator bool() const noexcept { return bool(m_data); }
  bool operator==(const deep_ptr<T> &other) const noexcept {
    return m_data == other.m_data;
  }
  bool operator!=(const deep_ptr<T> &other) const noexcept {
    return m_data != other.m_data;
  }

  const T &operator*() const { return *m_data; }
  T &operator*() { return *m_data; }
  const T *operator->() const { return m_data.get(); }
  T *operator->() { return m_data.get(); }

private:
  std::unique_ptr<T> m_data;
};

class Variable {
public:
  template <class T>
  Variable(uint32_t id, const Unit::Id unit, Dimensions dimensions, T object);

  const std::string &name() const {
    static const std::string empty;
    if (!m_name)
      return empty;
    return *m_name;
  }
  void setName(const std::string &name) {
    if (isCoord())
      throw std::runtime_error("Coordinate variable cannot have a name.");
    if (name.empty())
      m_name = nullptr;
    else if (m_name)
      *m_name = name;
    else
      m_name = std::make_unique<std::string>(name);
  }
  bool operator==(const Variable &other) const;
  bool operator!=(const Variable &other) const;
  Variable &operator+=(const Variable &other);
  Variable &operator-=(const Variable &other);
  Variable &operator*=(const Variable &other);
  void setSlice(const Variable &slice, const Dimension dim,
                const gsl::index index);

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
  void setDimensions(const Dimensions &dimensions);

  const VariableConcept &data() const { return *m_object; }
  VariableConcept &data() { return m_object.access(); }

  template <class Tag> bool valueTypeIs() const {
    return tag_id<Tag> == m_type;
  }

  uint16_t type() const { return m_type; }
  bool isCoord() const { return m_type < std::tuple_size<Coord::tags>::value; }
  bool isAttr() const {
    return m_type >= std::tuple_size<Coord::tags>::value +
                         std::tuple_size<Data::tags>::value;
  }
  bool isData() const { return !isCoord() && !isAttr(); }

  template <class Tag> auto get() const {
    static_assert(std::is_const<Tag>::value,
                  "Variable is `const`, must use const-qualified tag in call "
                  "to `get`, e.g., `get<const Coord::X>()` instead of "
                  "`get<Coord::X>()`");
    // For now we support only variables that are a std::vector. In principle we
    // could support anything that is convertible to gsl::span (or an adequate
    // replacement).
    return gsl::make_span(cast<Vector<typename Tag::type>>());
  }

  template <class Tag>
  auto get(std::enable_if_t<std::is_const<Tag>::value> * = nullptr) {
    return const_cast<const Variable *>(this)->get<Tag>();
  }

  template <class Tag>
  auto get(std::enable_if_t<!std::is_const<Tag>::value> * = nullptr) {
    return gsl::make_span(cast<Vector<typename Tag::type>>());
  }

private:
  template <class T> const T &cast() const;
  template <class T> T &cast();

  uint16_t m_type;
  Unit m_unit;
  deep_ptr<std::string> m_name;
  cow_ptr<VariableConcept> m_object;
};

template <class Tag, class... Args>
Variable makeVariable(Dimensions dimensions) {
  return Variable(tag_id<Tag>, Tag::unit, std::move(dimensions),
                  Vector<typename Tag::type>(dimensions.volume()));
}

template <class Tag, class... Args>
Variable makeVariable(Dimensions dimensions, Args &&... args) {
  return Variable(tag_id<Tag>, Tag::unit, std::move(dimensions),
                  Vector<typename Tag::type>(std::forward<Args>(args)...));
}

template <class Tag, class T>
Variable makeVariable(Dimensions dimensions, std::initializer_list<T> values) {
  return Variable(tag_id<Tag>, Tag::unit, std::move(dimensions),
                  Vector<typename Tag::type>(values.begin(), values.end()));
}

template <class Tag, class T>
Variable makeVariable(Dimensions dimensions, const std::vector<T> &values) {
  // Copy to aligned memory.
  return Variable(tag_id<Tag>, Tag::unit, std::move(dimensions),
                  Vector<typename Tag::type>(values.begin(), values.end()));
}

Variable operator+(Variable a, const Variable &b);
Variable operator-(Variable a, const Variable &b);
Variable operator*(Variable a, const Variable &b);

Variable slice(const Variable &var, const Dimension dim,
               const gsl::index index);
Variable concatenate(const Dimension dim, const Variable &a1,
                     const Variable &a2);

#endif // VARIABLE_H
