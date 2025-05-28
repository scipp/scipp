#include "scipp/variable/variable_concept.h"
#include "scipp/core/dimensions.h"

namespace scipp::variable {

VariableConcept::VariableConcept(const sc_units::Unit &unit) : m_unit(unit) {}

} // namespace scipp::variable
