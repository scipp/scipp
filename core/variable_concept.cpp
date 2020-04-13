#include "scipp/core/variable_concept.h"

#include "scipp/core/dimensions.h"

#include <utility>
#include <variant>

namespace scipp::core {
template <class... Known>
VariableConceptHandle_impl<Known...>::operator bool() const noexcept {
  return std::visit([](auto &&ptr) { return bool(ptr); }, m_object);
}

template <class... Known>
VariableConcept &VariableConceptHandle_impl<Known...>::operator*() const {
  return std::visit([](auto &&arg) -> VariableConcept & { return *arg; },
                    m_object);
}

template <class... Known>
VariableConcept *VariableConceptHandle_impl<Known...>::operator->() const {
  return std::visit(
      [](auto &&arg) -> VariableConcept * { return arg.operator->(); },
      m_object);
}

template <class... Known>
typename VariableConceptHandle_impl<Known...>::variant_t
VariableConceptHandle_impl<Known...>::variant() const noexcept {
  return std::visit(
      [](auto &&arg) {
        return std::variant<const VariableConcept *,
                            const VariableConceptT<Known> *...>{arg.get()};
      },
      m_object);
}

VariableConcept::VariableConcept(const Dimensions &dimensions)
    : m_dimensions(dimensions) {}

// Explicitly instansiate the template once in this TU
template class VariableConceptHandle_impl<KNOWN>;

} // namespace scipp::core
