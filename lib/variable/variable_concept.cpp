#include "scipp/variable/variable_concept.h"
#include "scipp/core/dimensions.h"

namespace scipp::variable {

VariableConcept::VariableConcept(const units::Unit &unit) : m_unit(unit) {}

} // namespace scipp::variable
