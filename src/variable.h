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
#include "variable_view.h"
#include "vector.h"

class Variable;

class VariableConcept {
public:
  VariableConcept(const Dimensions &dimensions);
  virtual ~VariableConcept() = default;
  // This is dropped into a cow_ptr so we prefer shared_ptr over unique_ptr.
  virtual std::shared_ptr<VariableConcept> clone() const = 0;
  virtual std::unique_ptr<VariableConcept> cloneUnique() const = 0;
  virtual std::shared_ptr<VariableConcept>
  clone(const Dimensions &dims) const = 0;
  virtual std::unique_ptr<VariableConcept> makeView() const = 0;
  virtual std::unique_ptr<VariableConcept> makeView() = 0;
  virtual std::unique_ptr<VariableConcept>
  makeView(const Dim dim, const gsl::index begin,
           const gsl::index end = -1) const = 0;
  virtual std::unique_ptr<VariableConcept>
  makeView(const Dim dim, const gsl::index begin,
           const gsl::index end = -1) = 0;
  virtual bool operator==(const VariableConcept &other) const = 0;

  virtual bool isContiguous() const = 0;
  virtual bool isView() const = 0;
  virtual bool isConstView() const = 0;

  virtual void rebin(const VariableConcept &old, const Dim dim,
                     const VariableConcept &oldCoord,
                     const VariableConcept &newCoord) = 0;
  virtual VariableConcept &operator+=(const VariableConcept &other) = 0;
  virtual VariableConcept &operator-=(const VariableConcept &other) = 0;
  virtual VariableConcept &operator*=(const VariableConcept &other) = 0;
  virtual gsl::index size() const = 0;
  virtual void copy(const VariableConcept &other, const Dim dim,
                    const gsl::index offset, const gsl::index otherBegin,
                    const gsl::index otherEnd) = 0;

  const Dimensions &dimensions() const { return m_dimensions; }

  friend class Variable;

private:
  Dimensions m_dimensions;
};

namespace detail {
template <class T> std::unique_ptr<T> clone(const T &other) {
  return std::make_unique<T>(other);
}

template <>
inline std::unique_ptr<VariableConcept> clone(const VariableConcept &other) {
  return other.cloneUnique();
}
} // namespace detail

/// Like std::unique_ptr, but copy causes a deep copy.
template <class T> class deep_ptr {
public:
  deep_ptr() = default;
  deep_ptr(std::unique_ptr<T> &&other) : m_data(std::move(other)) {}
  deep_ptr(const deep_ptr<T> &other)
      : m_data(other ? detail::clone(*other) : nullptr) {}
  deep_ptr(deep_ptr<T> &&) = default;
  constexpr deep_ptr(std::nullptr_t){};
  deep_ptr<T> &operator=(const deep_ptr<T> &other) {
    if (&other != this && other)
      m_data = detail::clone(*other);
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

  T &operator*() const { return *m_data; }
  T *operator->() const { return m_data.get(); }

private:
  std::unique_ptr<T> m_data;
};

template <class... Tags> class LinearView;
template <class V> class VariableSlice;
template <class Base> class VariableSliceMutableMixin;

class Variable {
public:
  // TODO Having this non-explicit is convenient when passing (potential)
  // variable slices to functions that do not support slices, but implicit
  // conversion may introduce risks, so there is a trade-of here.
  Variable(const VariableSlice<const Variable> &slice);
  Variable(const VariableSlice<Variable> &slice);

  template <class T>
  Variable(Tag tag, const Unit::Id unit, const Dimensions &dimensions,
           T object);

  template <class VarSlice> Variable &operator=(const VarSlice &slice);

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
  template <class T> bool operator==(const T &other) const;
  template <class T> bool operator!=(const T &other) const;
  template <class T> Variable &operator+=(const T &other);
  template <class T> Variable &operator-=(const T &other);
  template <class T> Variable &operator*=(const T &other);
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

  template <class Tag> bool valueTypeIs() const { return Tag{} == m_tag; }

  Tag tag() const { return m_tag; }
  bool isCoord() const {
    return m_tag < std::tuple_size<detail::CoordDef::tags>::value;
  }
  bool isAttr() const {
    return m_tag >= std::tuple_size<detail::CoordDef::tags>::value +
                        std::tuple_size<detail::DataDef::tags>::value;
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
    return gsl::make_span(cast<typename Tag::type>());
  }

  template <class Tag>
  auto get(std::enable_if_t<std::is_const<Tag>::value> * = nullptr) {
    return const_cast<const Variable *>(this)->get<Tag>();
  }

  template <class Tag>
  auto get(std::enable_if_t<!std::is_const<Tag>::value> * = nullptr) {
    // TODO There is a bug here... this is accepting any tag that has the
    // matching type.
    return gsl::make_span(cast<typename Tag::type>());
  }

  VariableSlice<const Variable> operator()(const Dim dim,
                                           const gsl::index begin,
                                           const gsl::index end = -1) const;

  VariableSlice<Variable> operator()(const Dim dim, const gsl::index begin,
                                     const gsl::index end = -1);

  template <class... Tags> friend class LinearView;
  template <class Base> friend class VariableSliceMutableMixin;

private:
  template <class T> const Vector<T> &cast() const;
  template <class T> Vector<T> &cast();
  // Used by LinearView. Need to find a better way instead of having everyone as
  // friend.
  Dimensions &mutableDimensions() { return m_object.access().m_dimensions; }

  Tag m_tag;
  Unit m_unit;
  deep_ptr<std::string> m_name;
  cow_ptr<VariableConcept> m_object;
};

template <class Tag> Variable makeVariable(const Dimensions &dimensions) {
  return Variable(Tag{}, Tag::unit, std::move(dimensions),
                  Vector<typename Tag::type>(dimensions.volume()));
}

template <class Tag, class... Args>
Variable makeVariable(const Dimensions &dimensions, Args &&... args) {
  return Variable(Tag{}, Tag::unit, std::move(dimensions),
                  Vector<typename Tag::type>(std::forward<Args>(args)...));
}

template <class Tag, class T>
Variable makeVariable(const Dimensions &dimensions,
                      std::initializer_list<T> values) {
  return Variable(Tag{}, Tag::unit, std::move(dimensions),
                  Vector<typename Tag::type>(values.begin(), values.end()));
}

template <class Tag, class T>
Variable makeVariable(const Dimensions &dimensions,
                      const std::vector<T> &values) {
  // Copy to aligned memory.
  return Variable(Tag{}, Tag::unit, std::move(dimensions),
                  Vector<typename Tag::type>(values.begin(), values.end()));
}

template <class Base> class VariableSliceMutableMixin {};

template <> class VariableSliceMutableMixin<VariableSlice<const Variable>> {
protected:
  template <class T> const VariableView<const T> &cast() const;

private:
  const VariableSlice<const Variable> &base() const;
  VariableSlice<const Variable> &base();
};

template <> class VariableSliceMutableMixin<VariableSlice<Variable>> {
public:
  template <class T>
  VariableSliceMutableMixin<VariableSlice<Variable>> &copyFrom(const T &other);
  template <class T> VariableSlice<Variable> &operator+=(const T &other);
  template <class T> VariableSlice<Variable> &operator-=(const T &other);
  template <class T> VariableSlice<Variable> &operator*=(const T &other);

  void setUnit(const Unit &unit);

protected:
  // Special version creating const view from mutable view. Note that this does
  // not return a reference but by value.
  template <class T> VariableView<const T> cast() const;
  template <class T> const VariableView<T> &cast();

private:
  const VariableSlice<Variable> &base() const;
  VariableSlice<Variable> &base();
};

// V is either Variable or const Variable.
// Note/TODO: The call to `data()` will trigger the copy-on-write mechanism.
// Const-correctness is therefore important when we want to create a slice view
// without triggering a copy unintentionally. This will also happen in Python we
// we do not have const-ness, so we need to figure out if this copy-triggering
// behavior is acceptable. An alternative may be to delay getting data until a
// mutating method of the slice view is called.
template <class V>
class VariableSlice : public VariableSliceMutableMixin<VariableSlice<V>> {
public:
  explicit VariableSlice(V &variable)
      : m_variable(&variable), m_view(variable.data().makeView()) {}
  VariableSlice(const VariableSlice &other) = default;
  VariableSlice(V &variable, const Dim dim, const gsl::index begin,
                const gsl::index end = -1)
      : m_variable(&variable),
        m_view(variable.data().makeView(dim, begin, end)) {}
  VariableSlice(const VariableSlice &slice, const Dim dim,
                const gsl::index begin, const gsl::index end = -1)
      : m_variable(slice.m_variable),
        m_view(slice.m_view->makeView(dim, begin, end)) {}

  VariableSlice operator()(const Dim dim, const gsl::index begin,
                           const gsl::index end = -1) const {
    return VariableSlice(*this, dim, begin, end);
  }

  const std::string &name() const { return m_variable->name(); }
  void setName(const std::string &) {
    throw std::runtime_error("Cannot rename Variable via slice view.");
  }
  const Unit &unit() const { return m_variable->unit(); }
  gsl::index size() const { return m_view->size(); }

  // TODO Currently VariableSlice is returned by value from dataset slices, so
  // we risk referencing a member of a temporary. Returning by value for rvalue
  // case. Need the same for some other methods?
  const Dimensions &dimensions() const & { return m_view->dimensions(); }
  Dimensions dimensions() const && { return m_view->dimensions(); }

  std::vector<gsl::index> strides() const {
    const auto parent = m_variable->dimensions();
    std::vector<gsl::index> strides;
    for (const auto &label : parent.labels())
      if (dimensions().contains(label))
        strides.emplace_back(parent.offset(label));
    return strides;
  }

  template <class Tag> bool valueTypeIs() const {
    return m_variable->template valueTypeIs<Tag>();
  }
  Tag tag() const { return m_variable->tag(); }
  const VariableConcept &data() const { return *m_view; }
  VariableConcept &data() { return *m_view; }

  bool isCoord() const { return m_variable->isCoord(); }
  bool isAttr() const { return m_variable->isAttr(); }
  bool isData() const { return m_variable->isData(); }

  template <class Tag> auto get() const {
    static_assert(
        std::is_const<Tag>::value,
        "VariableSlice is `const`, must use const-qualified tag in call "
        "to `get`, e.g., `get<const Coord::X>()` instead of "
        "`get<Coord::X>()`");
    return this->template cast<typename Tag::type>();
  }

  template <class Tag>
  auto get(std::enable_if_t<std::is_const<Tag>::value> * = nullptr) {
    return const_cast<const VariableSlice *>(this)->get<Tag>();
  }

  template <class Tag>
  auto get(std::enable_if_t<!std::is_const<Tag>::value> * = nullptr) {
    return this->template cast<typename Tag::type>();
  }

  template <class T> bool operator==(const T &other) const;
  template <class T> bool operator!=(const T &other) const;

private:
  friend class Variable;
  template <class Base> friend class VariableSliceMutableMixin;

  V *m_variable;
  deep_ptr<VariableConcept> m_view;
};

Variable operator+(Variable a, const Variable &b);
Variable operator-(Variable a, const Variable &b);
Variable operator*(Variable a, const Variable &b);

Variable slice(const Variable &var, const Dimension dim,
               const gsl::index index);
Variable slice(const Variable &var, const Dimension dim, const gsl::index begin,
               const gsl::index end);
std::vector<Variable> split(const Variable &var, const Dim dim,
                            const std::vector<gsl::index> &indices);
Variable concatenate(const Variable &a1, const Variable &a2,
                     const Dimension dim);
Variable rebin(const Variable &var, const Variable &oldCoord,
               const Variable &newCoord);
Variable permute(const Variable &var, const Dimension dim,
                 const std::vector<gsl::index> &indices);
Variable filter(const Variable &var, const Variable &filter);
Variable sum(const Variable &var, const Dim dim);
Variable mean(const Variable &var, const Dim dim);

#endif // VARIABLE_H
