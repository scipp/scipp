#include "variable.h"

bool Variable::operator==(const Variable &other) const {
  // Compare even before pointer comparison since data may be shared even if
  // names differ.
  if (m_name != other.m_name)
    return false;
  if (m_unit != other.m_unit)
    return false;
  // Trivial case: Pointers are equal
  if (m_object == other.m_object)
    return true;
  // Deep comparison
  if (m_type != other.m_type)
    return false;
  if (!(m_dimensions == other.m_dimensions))
    return false;
  return *m_object == *other.m_object;
}

Variable &Variable::operator+=(const Variable &other) {
  // Addition with different Variable type is supported, mismatch of underlying
  // element types is handled in VariableModel::operator+=.
  if (m_unit != other.m_unit)
    throw std::runtime_error("Cannot add Variables: Units do not match.");
  // TODO Should we support shape mismatch also in Variable, or is it sufficient
  // to have that supported in Dataset::operator+=? If we support it here we
  // must be careful to update the dimensions of Dataset.
  if (!(m_dimensions == other.m_dimensions))
    throw std::runtime_error("Cannot add Variables: Dimensions do not match.");
  // Note: Different name is ok for addition.

  m_object.access() += *other.m_object;
  return *this;
}

Variable concatenate(const Dimension dim, const Variable &a1,
                     const Variable &a2) {
  if (a1.type() != a2.type())
    throw std::runtime_error(
        "Cannot concatenate Variables: Data types do not match.");
  if (a1.unit() != a2.unit())
    throw std::runtime_error(
        "Cannot concatenate Variables: Units do not match.");
  if (a1.name() != a2.name())
    throw std::runtime_error(
        "Cannot concatenate Variables: Names do not match.");
  const auto &dims1 = a1.dimensions();
  const auto &dims2 = a2.dimensions();
  if (!(dims1 == dims2))
    throw std::runtime_error(
        "Cannot concatenate Variables: Dimensions do not match.");
  // Should we permit creation of ragged outputs if one dimension does not
  // match?
  auto out(a1);
  auto dims(dims1);
  if (dims.contains(dim)) {
    dims.resize(dim, dims1.size(dim) + dims2.size(dim));
    out.setDimensions(dims);
    auto offset = dims1.offset(dim) * dims1.size(dim);
    out.data().copyFrom(offset, 0, 2, a1.data());
    out.data().copyFrom(offset, 1, 2, a2.data());
  } else {
    dims.add(dim, 2);
    out.setDimensions(dims);
    out.data().copyFrom(a1.size(), 1, 2, a2.data());
  }
  return out;
}
