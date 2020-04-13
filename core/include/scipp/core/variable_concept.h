// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_VARIABLE_CONCEPT_H_
#define SCIPP_VARIABLE_CONCEPT_H_

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/common/span.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/dtype.h"
#include "scipp/core/element_array_view.h"

#include <Eigen/Dense>

#include <memory>
#include <variant>
#include <vector>

namespace scipp::core {

class Variable;
class VariableConcept;
template <class T> class VariableConceptT;

template <class... Known> class VariableConceptHandle_impl {
public:
  using variant_t =
      std::variant<const VariableConcept *, const VariableConceptT<Known> *...>;

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
    if (*this && other) {
      // Avoid allocation of new element_array if output is of correct shape.
      // This yields a 5x speedup in assignment operations of variables.
      auto &concept = **this;
      auto &otherConcept = *other;
      if (!concept.isView() && !otherConcept.isView() &&
          concept.dtype() == otherConcept.dtype() &&
          concept.dims() == otherConcept.dims() &&
          concept.hasVariances() == otherConcept.hasVariances()) {
        concept.copy(otherConcept, Dim::Invalid, 0, 0, 1);
        return *this;
      }
    }
    return *this = other ? other->clone() : VariableConceptHandle_impl();
  }

  explicit operator bool() const noexcept;
  VariableConcept &operator*() const;
  VariableConcept *operator->() const;

  const auto &mutableVariant() const noexcept { return m_object; }

  variant_t variant() const noexcept;

private:
  std::variant<std::unique_ptr<VariableConcept>,
               std::unique_ptr<VariableConceptT<Known>>...>
      m_object;
};

// Any item type that is listed here explicitly can be used with the templated
// `transform`, i.e., we can pass arbitrary functors/lambdas to process data.
#define KNOWN                                                                  \
  double, float, int64_t, int32_t, bool, Eigen::Vector3d, Eigen::Quaterniond,  \
      sparse_container<double>, sparse_container<float>,                       \
      sparse_container<int64_t>, sparse_container<int32_t>,                    \
      sparse_container<bool>, span<const double>, span<double>,                \
      span<const float>, span<float>, span<const int64_t>, span<int64_t>,      \
      span<const int32_t>, span<int32_t>

extern template class VariableConceptHandle_impl<KNOWN>;

using VariableConceptHandle = VariableConceptHandle_impl<KNOWN>;

/// Abstract base class for any data that can be held by Variable. Also used
/// to hold views to data by (Const)VariableView. This is using so-called
/// concept-based polymorphism, see talks by Sean Parent.
///
/// This is the most generic representation for a multi-dimensional array of
/// data. More operations are supportd by the partially-typed
/// VariableConceptT.
class SCIPP_CORE_EXPORT VariableConcept {
public:
  VariableConcept(const Dimensions &dimensions);
  virtual ~VariableConcept() = default;

  virtual DType dtype() const noexcept = 0;
  virtual VariableConceptHandle clone() const = 0;
  virtual VariableConceptHandle
  makeDefaultFromParent(const Dimensions &dims) const = 0;
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

  virtual VariableConceptHandle
  transpose(const std::vector<Dim> &dms) const = 0;
  virtual VariableConceptHandle transpose(const std::vector<Dim> &dms) = 0;

  virtual bool operator==(const VariableConcept &other) const = 0;
  virtual bool isSame(const VariableConcept &other) const = 0;

  virtual bool isContiguous() const = 0;
  virtual bool isView() const = 0;
  virtual bool isConstView() const = 0;
  virtual bool hasVariances() const noexcept = 0;

  virtual scipp::index size() const = 0;
  virtual void copy(const VariableConcept &other, const Dim dim,
                    const scipp::index offset, const scipp::index otherBegin,
                    const scipp::index otherEnd) = 0;

  virtual void setVariances(Variable &&variances) = 0;

  const Dimensions &dims() const { return m_dimensions; }

  friend class Variable;

private:
  Dimensions m_dimensions;
};

/// Partially typed implementation of VariableConcept. This is a common base
/// class for DataModel<T> and ViewModel<T>. The former holds data in a
/// contiguous array, whereas the latter is a (potentially non-contiguous) view
/// into the former. This base class implements functionality that is common to
/// both, for a specific T.
template <class T> class VariableConceptT : public VariableConcept {
public:
  using value_type = T;

  VariableConceptT(const Dimensions &dimensions)
      : VariableConcept(dimensions) {}

  DType dtype() const noexcept override { return scipp::core::dtype<T>; }
  static DType static_dtype() noexcept { return scipp::core::dtype<T>; }

  virtual scipp::span<T> values() = 0;
  virtual scipp::span<T> values(const Dim dim, const scipp::index begin,
                                const scipp::index end) = 0;
  virtual scipp::span<const T> values() const = 0;
  virtual scipp::span<const T> values(const Dim dim, const scipp::index begin,
                                      const scipp::index end) const = 0;
  virtual scipp::span<T> variances() = 0;
  virtual scipp::span<T> variances(const Dim dim, const scipp::index begin,
                                   const scipp::index end) = 0;
  virtual scipp::span<const T> variances() const = 0;
  virtual scipp::span<const T> variances(const Dim dim,
                                         const scipp::index begin,
                                         const scipp::index end) const = 0;
  virtual ElementArrayView<T> valuesView(const Dimensions &dims) = 0;
  virtual ElementArrayView<T> valuesView(const Dimensions &dims, const Dim dim,
                                         const scipp::index begin) = 0;
  virtual ElementArrayView<const T>
  valuesView(const Dimensions &dims) const = 0;
  virtual ElementArrayView<const T>
  valuesView(const Dimensions &dims, const Dim dim,
             const scipp::index begin) const = 0;
  virtual ElementArrayView<T> variancesView(const Dimensions &dims) = 0;
  virtual ElementArrayView<T> variancesView(const Dimensions &dims,
                                            const Dim dim,
                                            const scipp::index begin) = 0;
  virtual ElementArrayView<const T>
  variancesView(const Dimensions &dims) const = 0;
  virtual ElementArrayView<const T>
  variancesView(const Dimensions &dims, const Dim dim,
                const scipp::index begin) const = 0;
  virtual ElementArrayView<const T>
  valuesReshaped(const Dimensions &dims) const = 0;
  virtual ElementArrayView<T> valuesReshaped(const Dimensions &dims) = 0;
  virtual ElementArrayView<const T>
  variancesReshaped(const Dimensions &dims) const = 0;
  virtual ElementArrayView<T> variancesReshaped(const Dimensions &dims) = 0;

  virtual std::unique_ptr<VariableConceptT> copyT() const = 0;

  VariableConceptHandle
  makeDefaultFromParent(const Dimensions &dims) const override;

  VariableConceptHandle makeView() const override;

  VariableConceptHandle makeView() override;

  VariableConceptHandle makeView(const Dim dim, const scipp::index begin,
                                 const scipp::index end) const override;

  VariableConceptHandle makeView(const Dim dim, const scipp::index begin,
                                 const scipp::index end) override;

  VariableConceptHandle reshape(const Dimensions &dims) const override;

  VariableConceptHandle reshape(const Dimensions &dims) override;

  VariableConceptHandle transpose(const std::vector<Dim> &dims) const override;

  VariableConceptHandle transpose(const std::vector<Dim> &dims) override;

  bool operator==(const VariableConcept &other) const override;
  void copy(const VariableConcept &other, const Dim dim,
            const scipp::index offset, const scipp::index otherBegin,
            const scipp::index otherEnd) override;
};

} // namespace scipp::core

#endif // SCIPP_VARIABLE_CONCEPT_H_
