#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

void VariableFactory::emplace(const DType key,
                              std::unique_ptr<AbstractVariableMaker> maker) {
  m_makers.emplace(key, std::move(maker));
}

bool VariableFactory::contains(const DType key) const noexcept {
  return m_makers.find(key) != m_makers.end();
}

Variable VariableFactory::create(const DType key, const Dimensions &dims,
                                 const bool variances) const {
  return m_makers.at(key)->create(dims, variances);
}

VariableFactory &variableFactory() {
  static VariableFactory factory;
  return factory;
}

} // namespace scipp::variable
