#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

void VariableFactory::emplace(const DType key,
                              std::unique_ptr<AbstractVariableMaker> maker) {
  m_makers.emplace(key, std::move(maker));
}

bool VariableFactory::contains(const DType key) const noexcept {
  return m_makers.find(key) != m_makers.end();
}

DType VariableFactory::elem_dtype(const VariableConstView &var) const {
  return m_makers.at(var.dtype())->elem_dtype(var);
}
bool VariableFactory::hasVariances(const VariableConstView &var) const {
  return m_makers.at(var.dtype())->hasVariances(var);
}

VariableFactory &variableFactory() {
  static VariableFactory factory;
  return factory;
}

} // namespace scipp::variable
