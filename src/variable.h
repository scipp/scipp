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

#include "dimensions.h"
#include "tags.h"
#include "unit.h"
#include "variable_view.h"
#include "vector.h"

class Variable;

/// Abstract base class for any data that can be held by Variable. Also used to
/// hold views to data by (Const)VariableSlice. This is using so-called
/// concept-based polymorphism, see talks by Sean Parent.
///
/// This is the most generic representation for a multi-dimensional array of
/// data. Depending on the item type more functionality such as binary
/// operations is supported. Virtual methods for these are added in
/// child-classes. See, e.g., `ArithmeticVariableConcept`.
class VariableConcept {
public:
  VariableConcept(const Dimensions &dimensions);
  virtual ~VariableConcept() = default;

  virtual DType dtype() const noexcept = 0;

  virtual std::unique_ptr<VariableConcept> clone() const = 0;
  virtual std::unique_ptr<VariableConcept>
  clone(const Dimensions &dims) const = 0;
  virtual std::unique_ptr<VariableConcept> makeView() const = 0;
  virtual std::unique_ptr<VariableConcept> makeView() = 0;
  virtual std::unique_ptr<VariableConcept>
  makeView(const Dim dim, const gsl::index begin,
           const gsl::index end = -1) const = 0;
  virtual std::unique_ptr<VariableConcept>
  makeView(const Dim dim, const gsl::index begin,
           const gsl::index end = -1) = 0;

  virtual std::unique_ptr<VariableConcept>
  reshape(const Dimensions &dims) const = 0;
  virtual std::unique_ptr<VariableConcept> reshape(const Dimensions &dims) = 0;

  virtual bool operator==(const VariableConcept &other) const = 0;

  virtual bool isContiguous() const = 0;
  virtual bool isView() const = 0;
  virtual bool isConstView() const = 0;

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
  return other.clone();
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

template <class... Tags> class ZipView;
class ConstVariableSlice;
class VariableSlice;
template <class T1, class T2> T1 &plus_equals(T1 &, const T2 &);

/// Variable is a type-erased handle to any data structure representing a
/// multi-dimensional array. It has a name, a unit, and a set of named
/// dimensions.
class Variable {
public:
  // Having this non-explicit is convenient when passing (potential)
  // variable slices to functions that do not support slices, but implicit
  // conversion may introduce risks, so there is a trade-of here.
  Variable(const ConstVariableSlice &slice);

  template <class TagT>
  Variable(TagT tag, const Dimensions &dimensions)
      : Variable(tag, TagT::unit, std::move(dimensions),
                 Vector<typename TagT::type>(dimensions.volume())) {}
  template <class TagT>
  Variable(TagT tag, const Dimensions &dimensions,
           Vector<typename TagT::type> object)
      : Variable(tag, TagT::unit, std::move(dimensions), std::move(object)) {}
  template <class TagT, class... Args>
  Variable(TagT tag, const Dimensions &dimensions, Args &&... args)
      : Variable(tag, TagT::unit, std::move(dimensions),
                 Vector<typename TagT::type>(std::forward<Args>(args)...)) {}
  template <class TagT, class T>
  Variable(TagT tag, const Dimensions &dimensions,
           std::initializer_list<T> values)
      : Variable(tag, TagT::unit, std::move(dimensions),
                 Vector<typename TagT::type>(values.begin(), values.end())) {}
  template <class TagT, class T>
  Variable(TagT tag, const Dimensions &dimensions, const std::vector<T> &values)
      : Variable(tag, TagT::unit, std::move(dimensions),
                 // Copy to aligned memory.
                 Vector<typename TagT::type>(values.begin(), values.end())) {}

  template <class T>
  Variable(const Tag tag, const Unit::Id unit, const Dimensions &dimensions,
           T object);

  const std::string &name() const && = delete;
  const std::string &name() const & {
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
  bool operator==(const ConstVariableSlice &other) const;
  bool operator!=(const Variable &other) const;
  bool operator!=(const ConstVariableSlice &other) const;
  Variable operator-() const;
  Variable &operator+=(const Variable &other) &;
  Variable &operator+=(const ConstVariableSlice &other) &;
  Variable &operator+=(const double value) &;
  Variable &operator-=(const Variable &other) &;
  Variable &operator-=(const ConstVariableSlice &other) &;
  Variable &operator-=(const double value) &;
  Variable &operator*=(const Variable &other) &;
  Variable &operator*=(const ConstVariableSlice &other) &;
  Variable &operator*=(const double value) &;
  Variable &operator/=(const Variable &other) &;
  Variable &operator/=(const ConstVariableSlice &other) &;
  Variable &operator/=(const double value) &;

  Unit unit() const { return m_unit; }
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

  const Dimensions &dimensions() const && = delete;
  const Dimensions &dimensions() const & { return m_object->dimensions(); }
  void setDimensions(const Dimensions &dimensions);

  const VariableConcept &data() const && = delete;
  const VariableConcept &data() const & { return *m_object; }
  VariableConcept &data() && = delete;
  VariableConcept &data() & { return *m_object; }

  DType dtype() const noexcept { return data().dtype(); }
  Tag tag() const { return m_tag; }
  bool isCoord() const {
    return m_tag < std::tuple_size<detail::CoordDef::tags>::value;
  }
  bool isAttr() const {
    return m_tag >= std::tuple_size<detail::CoordDef::tags>::value +
                        std::tuple_size<detail::DataDef::tags>::value;
  }
  bool isData() const { return !isCoord() && !isAttr(); }

  template <class TagT> auto get(const TagT t) const {
    // For now we support only variables that are a std::vector. In principle we
    // could support anything that is convertible to gsl::span (or an adequate
    // replacement).
    if (t != tag())
      throw std::runtime_error("Attempt to access variable with wrong tag.");
    return gsl::make_span(cast<typename TagT::type>());
  }

  template <class TagT> auto get(const TagT t) {
    if (t != tag())
      throw std::runtime_error("Attempt to access variable with wrong tag.");
    return gsl::make_span(cast<typename TagT::type>());
  }

  template <class T> auto span() const { return gsl::make_span(cast<T>()); }
  template <class T> auto span() { return gsl::make_span(cast<T>()); }

  // ATTENTION: It is really important to delete any function returning a
  // (Const)VariableSlice for rvalue Variable. Otherwise the resulting slice
  // will point to free'ed memory.
  ConstVariableSlice operator()(const Dim dim, const gsl::index begin,
                                const gsl::index end = -1) const &;
  ConstVariableSlice operator()(const Dim dim, const gsl::index begin,
                                const gsl::index end = -1) const && = delete;

  VariableSlice operator()(const Dim dim, const gsl::index begin,
                           const gsl::index end = -1) &;
  VariableSlice operator()(const Dim dim, const gsl::index begin,
                           const gsl::index end = -1) && = delete;

  ConstVariableSlice reshape(const Dimensions &dims) const &;
  VariableSlice reshape(const Dimensions &dims) &;
  // Note: Do we have to delete the `const &&` version? Consider
  //   const Variable var;
  //   std::move(var).reshape({});
  // This calls `reshape() const &` but in this case it is not a temporary and
  // will not go out of scope, so that is ok (unless someone changes var and
  // expects the reshaped view to be still valid).
  Variable reshape(const Dimensions &dims) &&;

  template <class... Tags> friend class ZipView;
  template <class T1, class T2> friend T1 &plus_equals(T1 &, const T2 &);

private:
  template <class T> const Vector<T> &cast() const;
  template <class T> Vector<T> &cast();

  // Used by ZipView. Need to find a better way instead of having everyone as
  // friend.
  Dimensions &mutableDimensions() { return m_object->m_dimensions; }

  Tag m_tag;
  Unit m_unit;
  deep_ptr<std::string> m_name;
  deep_ptr<VariableConcept> m_object;
};

// TODO TagT template argument is not actually required, provided that we
// refactor tags so unit can be obtained from the `Tag` base class. If we do
// this, we can probably also unify a good amount of code in the Python exports,
// which is currently requiring exporting for all tags for many methods.
template <class T, class TagT>
Variable makeVariable(TagT tag, const Dimensions &dimensions) {
  return Variable(tag, TagT::unit, std::move(dimensions),
                  Vector<T>(dimensions.volume()));
}

template <class T, class TagT, class T2>
Variable makeVariable(TagT tag, const Dimensions &dimensions,
                      std::initializer_list<T2> values) {
  return Variable(tag, TagT::unit, std::move(dimensions),
                  Vector<T>(values.begin(), values.end()));
}

/// Non-mutable view into (a subset of) a Variable.
class ConstVariableSlice {
public:
  explicit ConstVariableSlice(const Variable &variable)
      : m_variable(&variable) {}
  ConstVariableSlice(const Variable &variable, const Dimensions &dims)
      : m_variable(&variable), m_view(variable.data().reshape(dims)) {}
  ConstVariableSlice(const ConstVariableSlice &other) = default;
  ConstVariableSlice(const Variable &variable, const Dim dim,
                     const gsl::index begin, const gsl::index end = -1)
      : m_variable(&variable),
        m_view(variable.data().makeView(dim, begin, end)) {}
  ConstVariableSlice(const ConstVariableSlice &slice, const Dim dim,
                     const gsl::index begin, const gsl::index end = -1)
      : m_variable(slice.m_variable),
        m_view(slice.data().makeView(dim, begin, end)) {}

  ConstVariableSlice operator()(const Dim dim, const gsl::index begin,
                                const gsl::index end = -1) const {
    return ConstVariableSlice(*this, dim, begin, end);
  }

  // Note the return type. Reshaping a non-contiguous slice cannot return a
  // slice in general so we must return a copy of the data.
  Variable reshape(const Dimensions &dims) const;

  const std::string &name() const { return m_variable->name(); }
  void setName(const std::string &) {
    throw std::runtime_error("Cannot rename Variable via slice view.");
  }
  Unit unit() const { return m_variable->unit(); }
  gsl::index size() const {
    if (m_view)
      return m_view->size();
    else
      return m_variable->size();
  }

  // Note: Returning by value to avoid issues with referencing a temporary
  // (VariableSlice is returned by-value from DatasetSlice).
  Dimensions dimensions() const {
    if (m_view)
      return m_view->dimensions();
    else
      return m_variable->dimensions();
  }

  std::vector<gsl::index> strides() const {
    const auto parent = m_variable->dimensions();
    std::vector<gsl::index> strides;
    for (const auto &label : parent.labels())
      if (dimensions().contains(label))
        strides.emplace_back(parent.offset(label));
    return strides;
  }

  DType dtype() const noexcept { return data().dtype(); }
  Tag tag() const { return m_variable->tag(); }
  const VariableConcept &data() const && = delete;
  const VariableConcept &data() const & {
    if (m_view)
      return *m_view;
    else
      return m_variable->data();
  }

  bool isCoord() const { return m_variable->isCoord(); }
  bool isAttr() const { return m_variable->isAttr(); }
  bool isData() const { return m_variable->isData(); }

  // Note: This return a proxy object (a VariableView) that does reference
  // members owner by *this. Therefore we can support this even for temporaries
  // and we do not need to delete the rvalue overload, unlike for many other
  // methods. The data is owned by the underlying variable so it will not be
  // deleted even if *this is a temporary and gets deleted.
  template <class TagT> auto get(const TagT t) const {
    if (t != tag())
      throw std::runtime_error("Attempt to access variable with wrong tag.");
    return this->template cast<typename TagT::type>();
  }

  template <class T> auto span() const { return cast<T>(); }

  bool operator==(const Variable &other) const;
  bool operator==(const ConstVariableSlice &other) const;
  bool operator!=(const Variable &other) const;
  bool operator!=(const ConstVariableSlice &other) const;
  Variable operator-() const;

protected:
  friend class Variable;
  template <class T1, class T2> friend T1 &plus_equals(T1 &, const T2 &);

  template <class T> const VariableView<const T> cast() const;

  const Variable *m_variable;
  deep_ptr<VariableConcept> m_view;
};

/** Mutable view into (a subset of) a Variable.
 *
 * By inheriting from ConstVariableSlice any code that works for
 * ConstVariableSlice will automatically work also for this mutable variant. */
class VariableSlice : public ConstVariableSlice {
public:
  explicit VariableSlice(Variable &variable)
      : ConstVariableSlice(variable), m_mutableVariable(&variable) {}
  // Note that we use the basic constructor of ConstVariableSlice to avoid
  // creation of a const m_view, which would be overwritten immediately.
  VariableSlice(Variable &variable, const Dimensions &dims)
      : ConstVariableSlice(variable), m_mutableVariable(&variable) {
    m_view = variable.data().reshape(dims);
  }
  VariableSlice(const VariableSlice &other) = default;
  VariableSlice(Variable &variable, const Dim dim, const gsl::index begin,
                const gsl::index end = -1)
      : ConstVariableSlice(variable), m_mutableVariable(&variable) {
    m_view = variable.data().makeView(dim, begin, end);
  }
  VariableSlice(const VariableSlice &slice, const Dim dim,
                const gsl::index begin, const gsl::index end = -1)
      : ConstVariableSlice(slice), m_mutableVariable(slice.m_mutableVariable) {
    m_view = slice.data().makeView(dim, begin, end);
  }

  VariableSlice operator()(const Dim dim, const gsl::index begin,
                           const gsl::index end = -1) const {
    return VariableSlice(*this, dim, begin, end);
  }

  void setName(const std::string &) {
    throw std::runtime_error("Cannot rename Variable via slice view.");
  }

  using ConstVariableSlice::data;
  using ConstVariableSlice::get;

  VariableConcept &data() const && = delete;
  VariableConcept &data() const & {
    if (!m_view)
      return m_mutableVariable->data();
    return *m_view;
  }

  // Note: No need to delete rvalue overloads here, see ConstVariableSlice.
  template <class TagT> auto get(const TagT t) const {
    if (t != tag())
      throw std::runtime_error("Attempt to access variable with wrong tag.");
    return this->template cast<typename TagT::type>();
  }

  template <class T> auto span() { return cast<T>(); }

  // Note: We want to support things like `var(Dim::X, 0) += var2`, i.e., when
  // the left-hand-side is a temporary. This is ok since data is modified in
  // underlying Variable. However, we do not return the typical `VariableSlice
  // &` from these operations since that could reference a temporary. Due to the
  // way Python implements things like __iadd__ we must return an object
  // referencing the data though. We therefore return by value (this is not for
  // free since it involves a memory allocation but is probably relatively cheap
  // compared to other things). If the return by value turns out to be a
  // performance issue, another option is to have overloads for *this of types
  // `&` and `&&` with distinct return types (by reference in the first case, by
  // value in the second). In principle we may also change the implementation of
  // the Python exports to return `a` after calling `a += b` instead of
  // returning `a += b` but I am not sure how Pybind11 handles object lifetimes
  // (would this suffer from the same issue?).
  template <class T> VariableSlice assign(const T &other) const;
  VariableSlice operator+=(const Variable &other) const;
  VariableSlice operator+=(const ConstVariableSlice &other) const;
  VariableSlice operator+=(const double value) const;
  VariableSlice operator-=(const Variable &other) const;
  VariableSlice operator-=(const ConstVariableSlice &other) const;
  VariableSlice operator-=(const double value) const;
  VariableSlice operator*=(const Variable &other) const;
  VariableSlice operator*=(const ConstVariableSlice &other) const;
  VariableSlice operator*=(const double value) const;
  VariableSlice operator/=(const Variable &other) const;
  VariableSlice operator/=(const ConstVariableSlice &other) const;
  VariableSlice operator/=(const double value) const;

  void setUnit(const Unit &unit) const;

private:
  friend class Variable;
  template <class... Tags> friend class ZipView;
  template <class T1, class T2> friend T1 &plus_equals(T1 &, const T2 &);

  template <class T> VariableView<T> cast() const;

  Variable *m_mutableVariable;
};

// Note: If the left-hand-side in an addition is a VariableSlice this simply
// implicitly converts it to a Variable. A copy for the return value is required
// anyway so this is a convenient way to avoid defining more overloads.
Variable operator+(Variable a, const Variable &b);
Variable operator-(Variable a, const Variable &b);
Variable operator*(Variable a, const Variable &b);
Variable operator/(Variable a, const Variable &b);
Variable operator+(Variable a, const ConstVariableSlice &b);
Variable operator-(Variable a, const ConstVariableSlice &b);
Variable operator*(Variable a, const ConstVariableSlice &b);
Variable operator/(Variable a, const ConstVariableSlice &b);
Variable operator+(Variable a, const double b);
Variable operator-(Variable a, const double b);
Variable operator*(Variable a, const double b);
Variable operator/(Variable a, const double b);
Variable operator+(const double a, Variable b);
Variable operator-(const double a, Variable b);
Variable operator*(const double a, Variable b);
Variable operator/(const double a, Variable b);

std::vector<Variable> split(const Variable &var, const Dim dim,
                            const std::vector<gsl::index> &indices);
Variable concatenate(const Variable &a1, const Variable &a2, const Dim dim);
Variable rebin(const Variable &var, const Variable &oldCoord,
               const Variable &newCoord);
Variable permute(const Variable &var, const Dim dim,
                 const std::vector<gsl::index> &indices);
Variable filter(const Variable &var, const Variable &filter);
Variable sum(const Variable &var, const Dim dim);
Variable mean(const Variable &var, const Dim dim);

template <class T>
VariableView<const T> getView(const Variable &var, const Dimensions &dims);

#endif // VARIABLE_H
