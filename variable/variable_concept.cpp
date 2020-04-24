#include "scipp/variable/variable_concept.h"
#include "scipp/core/dimensions.h"

namespace scipp::variable {

VariableConceptHandle::VariableConceptHandle(const VariableConceptHandle &other)
    : VariableConceptHandle(other ? other->clone() : VariableConceptHandle()) {}

VariableConceptHandle &
VariableConceptHandle::operator=(const VariableConceptHandle &other) {
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
  return *this = other ? other->clone() : VariableConceptHandle();
}

VariableConcept::VariableConcept(const Dimensions &dimensions)
    : m_dimensions(dimensions) {}

} // namespace scipp::variable
