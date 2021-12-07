// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include "scipp/core/dimensions.h"
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"
#include "scipp/units/unit.h"
#include "scipp/variable/element_array_model.h"
#include "scipp/variable/except.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable_concept.h"

namespace scipp::variable {

/// Implementation of VariableConcept that represents an array with structured
/// elements of type T.
///
/// The difference to ElementArrayModel is that this class allows for creating
/// variables that share ownership of the underlying structure elements, e.g.,
/// to provide access to an array of vector elements from an array of vectors.
template <class T, class Elem>
class StructureArrayModel : public VariableConcept {
public:
  using value_type = T;
  using element_type = Elem;
  static constexpr scipp::index element_count = sizeof(T) / sizeof(Elem);
  static_assert(sizeof(Elem) * element_count == sizeof(T));

  StructureArrayModel(const scipp::index size, const units::Unit &unit,
                      element_array<Elem> model)
      : VariableConcept(units::one), // unit ignored
        m_elements(std::make_shared<ElementArrayModel<Elem>>(
            size * element_count, unit, std::move(model))) {}

  StructureArrayModel(VariableConceptHandle &&elements)
      : VariableConcept(units::one), // unit ignored
        m_elements(std::move(elements)) {}

  static DType static_dtype() noexcept { return scipp::dtype<T>; }
  DType dtype() const noexcept override { return scipp::dtype<T>; }
  scipp::index size() const override {
    return m_elements->size() / element_count;
  }

  const units::Unit &unit() const override { return m_elements->unit(); }
  void setUnit(const units::Unit &unit) override { m_elements->setUnit(unit); }

  VariableConceptHandle
  makeDefaultFromParent(const scipp::index size) const override;

  VariableConceptHandle
  makeDefaultFromParent(const Variable &shape) const override {
    return makeDefaultFromParent(shape.dims().volume());
  }

  bool equals(const Variable &a, const Variable &b) const override;
  void copy(const Variable &src, Variable &dest) const override;
  void copy(const Variable &src, Variable &&dest) const override;
  void assign(const VariableConcept &other) override;

  bool has_variances() const noexcept override { return false; }
  void setVariances(const Variable &) override {
    except::throw_cannot_have_variances(core::dtype<T>);
  }

  VariableConceptHandle clone() const override {
    return std::make_shared<StructureArrayModel<T, Elem>>(m_elements->clone());
  }

  auto values(const core::ElementArrayViewParams &base) const {
    return ElementArrayView(base, get_values());
  }
  auto values(const core::ElementArrayViewParams &base) {
    return ElementArrayView(base, get_values());
  }

  VariableConceptHandle elements() const { return m_elements; }

  scipp::index dtype_size() const override { return sizeof(T); }
  const VariableConceptHandle &bin_indices() const override {
    throw except::TypeError("This data type does not have bin indices.");
  }

  scipp::span<const T> values() const {
    return {get_values(), static_cast<size_t>(size())};
  }
  scipp::span<T> values() {
    return {get_values(), static_cast<size_t>(size())};
  }

private:
  const T *get_values() const;
  T *get_values();
  VariableConceptHandle m_elements;
};

template <class T, class Elem>
VariableConceptHandle StructureArrayModel<T, Elem>::makeDefaultFromParent(
    const scipp::index size) const {
  return std::make_shared<StructureArrayModel<T, Elem>>(
      size, unit(), element_array<Elem>(size * element_count));
}

template <class T, class Elem>
bool StructureArrayModel<T, Elem>::equals(const Variable &a,
                                          const Variable &b) const {
  return a.dtype() == dtype() && b.dtype() == dtype() &&
         a.elements<T>() == b.elements<T>();
}

template <class T, class Elem>
void StructureArrayModel<T, Elem>::copy(const Variable &src,
                                        Variable &dest) const {
  transform_in_place<T>(
      dest, src, [](auto &a, const auto &b) { a = b; }, "copy");
}
template <class T, class Elem>
void StructureArrayModel<T, Elem>::copy(const Variable &src,
                                        Variable &&dest) const {
  copy(src, dest);
}

} // namespace scipp::variable
