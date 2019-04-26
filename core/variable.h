// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef VARIABLE_H
#define VARIABLE_H

#include <exception>
#include <string>
#include <type_traits>
#include <variant>

#include "dimensions.h"
#include "index.h"
#include "scipp/units/unit.h"
#include "span.h"
#include "tags.h"
#include "variable_view.h"
#include "vector.h"

namespace scipp::core {

template <class T>
using sparse_container = boost::container::small_vector<T, 8>;
template <class T> struct is_sparse_container : std::false_type {};
template <class T>
struct is_sparse_container<sparse_container<T>> : std::true_type {};

class Variable;
template <class... Known> class VariableConceptHandle_impl;
// Any item type that is listed here explicitly can be used with the templated
// `transform`, i.e., we can pass arbitrary functors/lambdas to process data.
using VariableConceptHandle =
    VariableConceptHandle_impl<double, float, int64_t, Eigen::Vector3d,
                               sparse_container<double>>;

/// Abstract base class for any data that can be held by Variable. Also used to
/// hold views to data by (Const)VariableSlice. This is using so-called
/// concept-based polymorphism, see talks by Sean Parent.
///
/// This is the most generic representation for a multi-dimensional array of
/// data. More operations are supportd by the partially-typed VariableConceptT.
class VariableConcept {
public:
  VariableConcept(const Dimensions &dimensions);
  virtual ~VariableConcept() = default;

  virtual DType dtype(bool sparse = false) const noexcept = 0;

  virtual VariableConceptHandle clone() const = 0;
  virtual VariableConceptHandle clone(const Dimensions &dims) const = 0;
  virtual VariableConceptHandle makeView() const = 0;
  virtual VariableConceptHandle makeView() = 0;
  virtual VariableConceptHandle makeView(const Dim dim,
                                         const scipp::index begin,
                                         const scipp::index end = -1) const = 0;
  virtual VariableConceptHandle makeView(const Dim dim,
                                         const scipp::index begin,
                                         const scipp::index end = -1) = 0;

  virtual VariableConceptHandle reshape(const Dimensions &dims) const = 0;
  virtual VariableConceptHandle reshape(const Dimensions &dims) = 0;

  virtual bool operator==(const VariableConcept &other) const = 0;

  virtual bool isContiguous() const = 0;
  virtual bool isView() const = 0;
  virtual bool isConstView() const = 0;

  virtual scipp::index size() const = 0;
  virtual void copy(const VariableConcept &other, const Dim dim,
                    const scipp::index offset, const scipp::index otherBegin,
                    const scipp::index otherEnd) = 0;

  const Dimensions &dimensions() const { return m_dimensions; }

  friend class Variable;

private:
  Dimensions m_dimensions;
};

template <class T> class VariableConceptT;

template <class T, typename Enable = void> struct concept {
  using type = VariableConcept;
  using typeT = VariableConceptT<T>;
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
  using value_type = T;

  VariableConceptT(const Dimensions &dimensions) : concept_t<T>(dimensions) {}

  DType dtype(bool sparse = false) const noexcept override {
    if (!sparse)
      return scipp::core::dtype<T>;
    if constexpr (is_sparse_container<T>::value)
      return scipp::core::dtype<typename T::value_type>;
    std::terminate();
  }
  static DType static_dtype() noexcept { return scipp::core::dtype<T>; }

  virtual scipp::span<T> getSpan() = 0;
  virtual scipp::span<T> getSpan(const Dim dim, const scipp::index begin,
                                 const scipp::index end) = 0;
  virtual scipp::span<const T> getSpan() const = 0;
  virtual scipp::span<const T> getSpan(const Dim dim, const scipp::index begin,
                                       const scipp::index end) const = 0;
  virtual VariableView<T> getView(const Dimensions &dims) = 0;
  virtual VariableView<T> getView(const Dimensions &dims, const Dim dim,
                                  const scipp::index begin) = 0;
  virtual VariableView<const T> getView(const Dimensions &dims) const = 0;
  virtual VariableView<const T> getView(const Dimensions &dims, const Dim dim,
                                        const scipp::index begin) const = 0;
  virtual VariableView<const T> getReshaped(const Dimensions &dims) const = 0;
  virtual VariableView<T> getReshaped(const Dimensions &dims) = 0;

  VariableConceptHandle makeView() const override;

  VariableConceptHandle makeView() override;

  VariableConceptHandle makeView(const Dim dim, const scipp::index begin,
                                 const scipp::index end) const override;

  VariableConceptHandle makeView(const Dim dim, const scipp::index begin,
                                 const scipp::index end) override;

  VariableConceptHandle reshape(const Dimensions &dims) const override;

  VariableConceptHandle reshape(const Dimensions &dims) override;

  bool operator==(const VariableConcept &other) const override;
  void copy(const VariableConcept &other, const Dim dim,
            const scipp::index offset, const scipp::index otherBegin,
            const scipp::index otherEnd) override;
};

template <class... Known> class VariableConceptHandle_impl {
public:
  VariableConceptHandle_impl()
      : m_object(std::unique_ptr<VariableConcept>(nullptr)) {}
  template <class T> VariableConceptHandle_impl(T object) {
    using value_t = typename T::element_type::value_type;
    if constexpr ((std::is_same_v<value_t, Known> || ...))
      m_object = std::unique_ptr<VariableConceptT<value_t>>(std::move(object));
    else
      m_object = std::unique_ptr<VariableConcept>(std::move(object));
  }
  VariableConceptHandle_impl(VariableConceptHandle_impl &&) = default;
  VariableConceptHandle_impl(const VariableConceptHandle_impl &other)
      : VariableConceptHandle_impl(other ? other->clone()
                                         : VariableConceptHandle_impl()) {}
  VariableConceptHandle_impl &
  operator=(VariableConceptHandle_impl &&) = default;
  VariableConceptHandle_impl &
  operator=(const VariableConceptHandle_impl &other) {
    return *this = other ? other->clone() : VariableConceptHandle_impl();
  }

  explicit operator bool() const noexcept {
    return std::visit([](auto &&ptr) { return bool(ptr); }, m_object);
  }
  VariableConcept &operator*() const {
    return std::visit([](auto &&arg) -> VariableConcept & { return *arg; },
                      m_object);
  }
  VariableConcept *operator->() const {
    return std::visit(
        [](auto &&arg) -> VariableConcept * { return arg.operator->(); },
        m_object);
  }

  const auto &variant() const noexcept { return m_object; }

private:
  std::variant<std::unique_ptr<VariableConcept>,
               std::unique_ptr<VariableConceptT<Known>>...>
      m_object;
};

template <class... Tags> class ZipView;
class ConstVariableSlice;
class VariableSlice;
template <class T1, class T2> T1 &plus_equals(T1 &, const T2 &);

namespace detail {
template <class T> struct default_init {
  static T value() { return T(); }
};
// Eigen does not zero-initialize matrices (vectors), which is a recurrent
// source of bugs. Variable does zero-init instead.
template <class T, int Rows, int Cols>
struct default_init<Eigen::Matrix<T, Rows, Cols>> {
  static Eigen::Matrix<T, Rows, Cols> value() {
    return Eigen::Matrix<T, Rows, Cols>::Zero();
  }
};
} // namespace detail

/// Variable is a type-erased handle to any data structure representing a
/// multi-dimensional array. It has a name, a unit, and a set of named
/// dimensions.
class Variable {
public:
  // Having this non-explicit is convenient when passing (potential)
  // variable slices to functions that do not support slices, but implicit
  // conversion may introduce risks, so there is a trade-of here.
  Variable(const ConstVariableSlice &slice);
  Variable(const Variable &parent, const Dimensions &dims);
  Variable(const ConstVariableSlice &parent, const Dimensions &dims);
  Variable(const Variable &parent, VariableConceptHandle data);

  template <class TagT>
  Variable(TagT tag, const Dimensions &dimensions)
      : Variable(tag, TagT::unit, std::move(dimensions),
                 Vector<underlying_type_t<typename TagT::type>>(
                     dimensions.volume(),
                     detail::default_init<
                         underlying_type_t<typename TagT::type>>::value())) {}
  template <class TagT>
  Variable(TagT tag, const Dimensions &dimensions,
           Vector<underlying_type_t<typename TagT::type>> object)
      : Variable(tag, TagT::unit, std::move(dimensions), std::move(object)) {}
  template <class TagT, class... Args>
  Variable(TagT tag, const Dimensions &dimensions, Args &&... args)
      : Variable(tag, TagT::unit, std::move(dimensions),
                 Vector<underlying_type_t<typename TagT::type>>(
                     std::forward<Args>(args)...)) {}
  template <class TagT, class T>
  Variable(TagT tag, const Dimensions &dimensions,
           std::initializer_list<T> values)
      : Variable(tag, TagT::unit, std::move(dimensions),
                 Vector<underlying_type_t<typename TagT::type>>(values.begin(),
                                                                values.end())) {
  }
  template <class TagT, class T>
  Variable(TagT tag, const Dimensions &dimensions, const std::vector<T> &values)
      : Variable(tag, TagT::unit, std::move(dimensions),
                 // Copy to aligned memory.
                 Vector<underlying_type_t<typename TagT::type>>(values.begin(),
                                                                values.end())) {
  }

  template <class T>
  Variable(const Tag tag, const units::Unit unit, const Dimensions &dimensions,
           T object, const Dim sparseDim = Dim::Invalid);

  const std::string &name() const && = delete;
  const std::string &name() const & { return m_name; }
  void setName(const std::string &name) { m_name = name; }
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
  template <class T>
  Variable &operator*=(const boost::units::quantity<T> &quantity) & {
    setUnit(unit() * units::Unit(T{}));
    return *this *= quantity.value();
  }
  Variable &operator/=(const Variable &other) &;
  Variable &operator/=(const ConstVariableSlice &other) &;
  Variable &operator/=(const double value) &;
  template <class T>
  Variable &operator/=(const boost::units::quantity<T> &quantity) & {
    setUnit(unit() / units::Unit(T{}));
    return *this /= quantity.value();
  }

  units::Unit unit() const { return m_unit; }
  void setUnit(const units::Unit &unit) {
    // TODO
    // Some variables are special, e.g., Data::Tof, which must always have a
    // time-of-flight-related unit. We need some sort of check here. Is there a
    // better mechanism to implement this that does not require gatekeeping here
    // but expresses itself on the interface instead? Does it make sense to
    // handle all unit changes by conversion functions?
    m_unit = unit;
  }

  scipp::index size() const { return m_object->size(); }

  const Dimensions &dimensions() const && = delete;
  const Dimensions &dimensions() const & { return m_object->dimensions(); }
  void setDimensions(const Dimensions &dimensions);

  const VariableConcept &data() const && = delete;
  const VariableConcept &data() const & { return *m_object; }
  VariableConcept &data() && = delete;
  VariableConcept &data() & { return *m_object; }

  const VariableConceptHandle &dataHandle() const && = delete;
  const VariableConceptHandle &dataHandle() const & { return m_object; }

  DType dtype() const noexcept { return data().dtype(isSparse()); }
  Tag tag() const { return m_tag; }
  void setTag(const Tag tag) { m_tag = tag; }
  bool isCoord() const {
    return m_tag < std::tuple_size<detail::CoordDef::tags>::value;
  }
  bool isAttr() const {
    return m_tag >= std::tuple_size<detail::CoordDef::tags>::value +
                        std::tuple_size<detail::DataDef::tags>::value;
  }
  bool isData() const { return !isCoord() && !isAttr(); }

  bool isSparse() const noexcept { return m_sparseDim != Dim::Invalid; }
  Dim sparseDim() const { return m_sparseDim; }

  template <class TagT> auto get(const TagT t) const {
    // For now we support only variables that are a std::vector. In principle we
    // could support anything that is convertible to scipp::span (or an adequate
    // replacement).
    if (t != tag())
      throw std::runtime_error("Attempt to access variable with wrong tag.");
    return scipp::span(cast<typename TagT::type>());
  }

  template <class TagT> auto get(const TagT t) {
    if (t != tag())
      throw std::runtime_error("Attempt to access variable with wrong tag.");
    return scipp::span(cast<typename TagT::type>());
  }

  template <class T> auto span() const { return scipp::span(cast<T>()); }
  template <class T> auto span() { return scipp::span(cast<T>()); }
  template <class T> auto sparseSpan() const {
    return scipp::span(cast<sparse_container<T>>());
  }
  template <class T> auto sparseSpan() {
    return scipp::span(cast<sparse_container<T>>());
  }

  // ATTENTION: It is really important to delete any function returning a
  // (Const)VariableSlice for rvalue Variable. Otherwise the resulting slice
  // will point to free'ed memory.
  ConstVariableSlice operator()(const Dim dim, const scipp::index begin,
                                const scipp::index end = -1) const &;
  ConstVariableSlice operator()(const Dim dim, const scipp::index begin,
                                const scipp::index end = -1) const && = delete;

  VariableSlice operator()(const Dim dim, const scipp::index begin,
                           const scipp::index end = -1) &;
  VariableSlice operator()(const Dim dim, const scipp::index begin,
                           const scipp::index end = -1) && = delete;

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
  template <class T> const Vector<underlying_type_t<T>> &cast() const;
  template <class T> Vector<underlying_type_t<T>> &cast();

  // Used by ZipView. Need to find a better way instead of having everyone as
  // friend.
  Dimensions &mutableDimensions() { return m_object->m_dimensions; }

  Tag m_tag;
  Dim m_sparseDim{Dim::Invalid};
  units::Unit m_unit;
  std::string m_name;
  VariableConceptHandle m_object;
};

template <class T>
Variable makeVariable(Tag tag, const Dimensions &dimensions) {
  return Variable(tag, defaultUnit(tag), std::move(dimensions),
                  Vector<underlying_type_t<T>>(
                      dimensions.volume(),
                      detail::default_init<underlying_type_t<T>>::value()));
}

template <class T>
Variable makeSparseVariable(Tag tag, const Dimensions &dimensions,
                            const Dim sparseDim) {
  return Variable(
      tag, defaultUnit(tag), std::move(dimensions),
      Vector<sparse_container<underlying_type_t<T>>>(dimensions.volume()),
      sparseDim);
}

template <class T, class T2>
Variable makeVariable(Tag tag, const Dimensions &dimensions,
                      std::initializer_list<T2> values) {
  return Variable(tag, defaultUnit(tag), std::move(dimensions),
                  Vector<underlying_type_t<T>>(values.begin(), values.end()));
}

namespace detail {
template <class... N> struct is_vector : std::false_type {};
template <class N, class A>
struct is_vector<std::vector<N, A>> : std::true_type {};
} // namespace detail

template <class T, class... Args>
Variable makeVariable(Tag tag, const Dimensions &dimensions, Args &&... args) {
  // Note: Using `if constexpr` instead of another overload, since overloading
  // on universal reference arguments is problematic.
  if constexpr (detail::is_vector<std::remove_cv_t<
                    std::remove_reference_t<Args>>...>::value) {
    // Copies to aligned memory.
    return Variable(tag, defaultUnit(tag), std::move(dimensions),
                    Vector<underlying_type_t<T>>(args.begin(), args.end())...);
  } else {
    return Variable(tag, defaultUnit(tag), std::move(dimensions),
                    Vector<underlying_type_t<T>>(std::forward<Args>(args)...));
  }
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
                     const scipp::index begin, const scipp::index end = -1)
      : m_variable(&variable),
        m_view(variable.data().makeView(dim, begin, end)) {}
  ConstVariableSlice(const ConstVariableSlice &slice, const Dim dim,
                     const scipp::index begin, const scipp::index end = -1)
      : m_variable(slice.m_variable),
        m_view(slice.data().makeView(dim, begin, end)) {}

  ConstVariableSlice operator()(const Dim dim, const scipp::index begin,
                                const scipp::index end = -1) const {
    return ConstVariableSlice(*this, dim, begin, end);
  }

  // Note the return type. Reshaping a non-contiguous slice cannot return a
  // slice in general so we must return a copy of the data.
  Variable reshape(const Dimensions &dims) const;

  const std::string &name() const { return m_variable->name(); }
  void setName(const std::string &) {
    throw std::runtime_error("Cannot rename Variable via slice view.");
  }
  units::Unit unit() const { return m_variable->unit(); }
  scipp::index size() const {
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

  std::vector<scipp::index> strides() const {
    const auto parent = m_variable->dimensions();
    std::vector<scipp::index> strides;
    for (const auto &label : parent.labels())
      if (dimensions().contains(label))
        strides.emplace_back(parent.offset(label));
    return strides;
  }

  DType dtype() const noexcept { return m_variable->dtype(); }
  Tag tag() const { return m_variable->tag(); }

  const VariableConcept &data() const && = delete;
  const VariableConcept &data() const & {
    if (m_view)
      return *m_view;
    else
      return m_variable->data();
  }

  const VariableConceptHandle &dataHandle() const && = delete;
  const VariableConceptHandle &dataHandle() const & {
    if (m_view)
      return m_view;
    else
      return m_variable->dataHandle();
  }

  bool isCoord() const { return m_variable->isCoord(); }
  bool isAttr() const { return m_variable->isAttr(); }
  bool isData() const { return m_variable->isData(); }

  bool isSparse() const noexcept { return m_variable->isSparse(); }
  Dim sparseDim() const { return m_variable->sparseDim(); }

  // Note: This return a proxy object (a VariableView) that does reference
  // members owner by *this. Therefore we can support this even for
  // temporaries and we do not need to delete the rvalue overload, unlike for
  // many other methods. The data is owned by the underlying variable so it
  // will not be deleted even if *this is a temporary and gets deleted.
  template <class TagT> auto get(const TagT t) const {
    if (t != tag())
      throw std::runtime_error("Attempt to access variable with wrong tag.");
    return this->template cast<typename TagT::type>();
  }

  template <class T> auto span() const { return cast<T>(); }
  template <class T> auto sparseSpan() const {
    return cast<sparse_container<T>>();
  }

  bool operator==(const Variable &other) const;
  bool operator==(const ConstVariableSlice &other) const;
  bool operator!=(const Variable &other) const;
  bool operator!=(const ConstVariableSlice &other) const;
  Variable operator-() const;

protected:
  friend class Variable;
  template <class T1, class T2> friend T1 &plus_equals(T1 &, const T2 &);

  template <class T>
  const VariableView<const underlying_type_t<T>> cast() const;

  const Variable *m_variable;
  VariableConceptHandle m_view;
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
  VariableSlice(Variable &variable, const Dim dim, const scipp::index begin,
                const scipp::index end = -1)
      : ConstVariableSlice(variable), m_mutableVariable(&variable) {
    m_view = variable.data().makeView(dim, begin, end);
  }
  VariableSlice(const VariableSlice &slice, const Dim dim,
                const scipp::index begin, const scipp::index end = -1)
      : ConstVariableSlice(slice), m_mutableVariable(slice.m_mutableVariable) {
    m_view = slice.data().makeView(dim, begin, end);
  }

  VariableSlice operator()(const Dim dim, const scipp::index begin,
                           const scipp::index end = -1) const {
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

  const VariableConceptHandle &dataHandle() const && = delete;
  const VariableConceptHandle &dataHandle() const & {
    if (!m_view)
      return m_mutableVariable->dataHandle();
    return m_view;
  }

  // Note: No need to delete rvalue overloads here, see ConstVariableSlice.
  template <class TagT> auto get(const TagT t) const {
    if (t != tag())
      throw std::runtime_error("Attempt to access variable with wrong tag.");
    return this->template cast<typename TagT::type>();
  }

  template <class T> auto span() const { return cast<T>(); }
  template <class T> auto sparseSpan() const {
    return cast<sparse_container<T>>();
  }

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

  void setUnit(const units::Unit &unit) const;

private:
  friend class Variable;
  template <class... Tags> friend class ZipView;
  template <class T1, class T2> friend T1 &plus_equals(T1 &, const T2 &);

  template <class T> VariableView<underlying_type_t<T>> cast() const;

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
template <class T>
Variable operator*(Variable a, const boost::units::quantity<T> &quantity) {
  return std::move(a *= quantity);
}
template <class T>
Variable operator/(Variable a, const boost::units::quantity<T> &quantity) {
  return std::move(a /= quantity);
}

std::vector<Variable> split(const Variable &var, const Dim dim,
                            const std::vector<scipp::index> &indices);
Variable concatenate(const Variable &a1, const Variable &a2, const Dim dim);
Variable rebin(const Variable &var, const Variable &oldCoord,
               const Variable &newCoord);
Variable permute(const Variable &var, const Dim dim,
                 const std::vector<scipp::index> &indices);
Variable filter(const Variable &var, const Variable &filter);
Variable sum(const Variable &var, const Dim dim);
Variable mean(const Variable &var, const Dim dim);
Variable abs(const Variable &var);
Variable norm(const Variable &var);
// TODO add to dataset and python
Variable sqrt(const Variable &var);
Variable broadcast(Variable var, const Dimensions &dims);
Variable reverse(Variable var, const Dim dim);

template <class T>
VariableView<const T> getView(const Variable &var, const Dimensions &dims);

} // namespace scipp::core

#endif // VARIABLE_H
